module.exports = [
  {
    type: "heading",
    defaultValue: "Mainsail Monitor",
  },
  {
    type: "section",
    items: [
      {
        type: "heading",
        defaultValue: "Printer 1",
      },
      {
        type: "input",
        messageKey: "Printer1Name",
        defaultValue: "Printer 1",
        label: "Name",
        attributes: {
          placeholder: "Voron",
        },
      },
      {
        type: "input",
        messageKey: "Printer1Url",
        defaultValue: "http://printer.lan",
        label: "Moonraker URL",
        description:
          "The address of your printer, e.g. http://192.168.1.100 or http://hostname.local",
        attributes: {
          placeholder: "http://",
          type: "url",
        },
      },
    ],
  },
  {
    type: "section",
    items: [
      {
        type: "heading",
        defaultValue: "Printer 2",
      },
      {
        type: "input",
        messageKey: "Printer2Name",
        defaultValue: "",
        label: "Name",
        attributes: {
          placeholder: "Optional",
        },
      },
      {
        type: "input",
        messageKey: "Printer2Url",
        defaultValue: "",
        label: "Moonraker URL",
        attributes: {
          placeholder: "http://",
          type: "url",
        },
      },
    ],
  },
  {
    type: "section",
    items: [
      {
        type: "heading",
        defaultValue: "Printer 3",
      },
      {
        type: "input",
        messageKey: "Printer3Name",
        defaultValue: "",
        label: "Name",
        attributes: {
          placeholder: "Optional",
        },
      },
      {
        type: "input",
        messageKey: "Printer3Url",
        defaultValue: "",
        label: "Moonraker URL",
        attributes: {
          placeholder: "http://",
          type: "url",
        },
      },
    ],
  },
  {
    type: "submit",
    defaultValue: "Save Settings",
  },
];
