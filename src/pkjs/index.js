const MOONRAKER_URL = "http://v0.cardiff.lan";
const POLL_INTERVAL_MS = 10000;

var pollTimer = null;

function fetchPrinterStatus() {
  var url =
    MOONRAKER_URL +
    "/printer/objects/query" +
    "?extruder=temperature,target" +
    "&heater_bed=temperature,target" +
    "&print_stats=state,filename,print_duration" +
    "&virtual_sdcard=progress";

  var req = new XMLHttpRequest();
  req.onload = function () {
    try {
      var json = JSON.parse(this.responseText);
      var status = json.result.status;

      var nozzleTemp = Math.round(status.extruder.temperature);
      var nozzleTarget = Math.round(status.extruder.target);
      var bedTemp = Math.round(status.heater_bed.temperature);
      var bedTarget = Math.round(status.heater_bed.target);

      var printState = status.print_stats.state || "unknown";
      var progress = Math.round(status.virtual_sdcard.progress * 100);

      // Estimate time remaining from progress and elapsed duration
      var printDuration = status.print_stats.print_duration || 0;
      var timeLeft = 0;
      if (progress > 0 && printState === "printing") {
        var totalEstimate = printDuration / (progress / 100);
        timeLeft = Math.round(totalEstimate - printDuration);
      }

      var dict = {
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
          console.log("Data sent to watch " + nozzleTemp);
        },
        function (e) {
          console.log("Send failed: " + JSON.stringify(e));
        }
      );
    } catch (err) {
      console.log("Error parsing Moonraker response: " + err.message);
    }
  };

  req.onerror = function () {
    console.log("XHR error - is Moonraker reachable?");
    // Send an error state so the watch knows
    Pebble.sendAppMessage({ PrintState: "error" });
  };

  req.open("GET", url);
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

Pebble.addEventListener("ready", function () {
  console.log("PebbleKit JS ready - starting Klipper Pebble");
  startPolling();
});

Pebble.addEventListener("appmessage", function (e) {
  var dict = e.payload;
  if (dict["RequestUpdate"]) {
    console.log("Manual refresh requested");
    fetchPrinterStatus();
  }
});
