var Clay = require('@rebble/clay');
var clayConfig = require('./config');
new Clay(clayConfig);

const POLL_INTERVAL_MS = 10000;
const MAX_PRINTERS = 3;

var pollTimer = null;
var selectedPrinterIndex = 0;

function getSettings() {
  var settings = localStorage.getItem('clay-settings');
  if (settings) {
    try {
      return JSON.parse(settings);
    } catch (e) {}
  }
  return {};
}

function cleanUrl(url) {
  return (url || '').replace(/\/+$/, '');
}

function getConfiguredPrinters() {
  var settings = getSettings();
  var printers = [];

  for (var i = 1; i <= MAX_PRINTERS; i++) {
    var url = cleanUrl(settings['Printer' + i + 'Url']);
    if (url) {
      printers.push({
        name: settings['Printer' + i + 'Name'] || 'Printer ' + i,
        url: url,
      });
    }
  }

  if (printers.length === 0 && settings.MoonrakerUrl) {
    printers.push({
      name: settings.Printer1Name || 'Printer 1',
      url: cleanUrl(settings.MoonrakerUrl),
    });
  }

  return printers;
}

function restoreSelectedPrinterIndex(printers) {
  var storedIndex = parseInt(localStorage.getItem('selected-printer-index'), 10);
  if (!isNaN(storedIndex)) {
    selectedPrinterIndex = storedIndex;
  }

  if (selectedPrinterIndex < 0 || selectedPrinterIndex >= printers.length) {
    selectedPrinterIndex = 0;
  }
}

function getSelectedPrinter() {
  var printers = getConfiguredPrinters();
  restoreSelectedPrinterIndex(printers);

  return {
    printer: printers[selectedPrinterIndex] || null,
    index: printers.length > 0 ? selectedPrinterIndex : 0,
    count: printers.length,
  };
}

function switchToNextPrinter() {
  var printers = getConfiguredPrinters();
  if (printers.length <= 1) {
    return;
  }

  selectedPrinterIndex = (selectedPrinterIndex + 1) % printers.length;
  localStorage.setItem('selected-printer-index', selectedPrinterIndex);
  fetchPrinterStatus();
}

function sendPrinterState(state) {
  var printerName = state.printer ? state.printer.name : 'No printer';
  Pebble.sendAppMessage(
    {
      PrinterName: printerName,
      PrinterIndex: state.index,
      PrinterCount: state.count,
      PrintState: state.printer ? 'error' : 'config',
    },
    function () {
      console.log('Printer state sent to watch');
    },
    function (e) {
      console.log('Send failed: ' + JSON.stringify(e));
    }
  );
}

function fetchPrinterStatus() {
  var selected = getSelectedPrinter();
  if (!selected.printer) {
    console.log('No Moonraker URLs configured');
    sendPrinterState(selected);
    return;
  }

  var url =
    selected.printer.url +
    '/printer/objects/query' +
    '?extruder=temperature,target' +
    '&heater_bed=temperature,target' +
    '&print_stats=state,filename,print_duration' +
    '&virtual_sdcard=progress';

  var req = new XMLHttpRequest();
  req.onload = function () {
    try {
      var json = JSON.parse(this.responseText);
      var status = json.result.status;

      var nozzleTemp = Math.round(status.extruder.temperature);
      var nozzleTarget = Math.round(status.extruder.target);
      var bedTemp = Math.round(status.heater_bed.temperature);
      var bedTarget = Math.round(status.heater_bed.target);

      var printState = status.print_stats.state || 'unknown';
      var progress = Math.round(status.virtual_sdcard.progress * 100);

      // Estimate time remaining from progress and elapsed duration
      var printDuration = status.print_stats.print_duration || 0;
      var timeLeft = 0;
      if (progress > 0 && printState === 'printing') {
        var totalEstimate = printDuration / (progress / 100);
        timeLeft = Math.round(totalEstimate - printDuration);
      }

      var dict = {
        PrinterName: selected.printer.name,
        PrinterIndex: selected.index,
        PrinterCount: selected.count,
        NozzleTemp: nozzleTemp,
        NozzleTarget: nozzleTarget,
        BedTemp: bedTemp,
        BedTarget: bedTarget,
        PrintState: printState,
        PrintProgress: progress,
        PrintTimeLeft: timeLeft,
      };

      Pebble.sendAppMessage(
        dict,
        function () {
          console.log('Data sent to watch');
        },
        function (e) {
          console.log('Send failed: ' + JSON.stringify(e));
        }
      );
    } catch (err) {
      console.log('Error parsing Moonraker response: ' + err.message);
    }
  };

  req.onerror = function () {
    console.log('XHR error - is ' + selected.printer.name + ' reachable?');
    sendPrinterState(selected);
  };

  req.open('GET', url);
  req.send();
}

function startPolling() {
  fetchPrinterStatus();
  pollTimer = setInterval(fetchPrinterStatus, POLL_INTERVAL_MS);
}

function stopPolling() {
  if (pollTimer) {
    clearInterval(pollTimer);
    pollTimer = null;
  }
}

Pebble.addEventListener('ready', function () {
  console.log('PebbleKit JS ready - starting Klipper Monitor');
  startPolling();
});

Pebble.addEventListener('appmessage', function (e) {
  var dict = e.payload;
  if (dict['RequestUpdate']) {
    console.log('Manual refresh requested');
    fetchPrinterStatus();
  }
  if (dict['SelectPrinter']) {
    console.log('Switch printer requested');
    switchToNextPrinter();
  }
});
