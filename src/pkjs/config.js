module.exports = [
  {
    "type": "heading",
    "defaultValue": "Shards watchface configuration"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Colors"
      },
      {
        "type": "color",
        "messageKey": "BACKGROUND_COLOR",
        "defaultValue": "0x000000",
        "label": "Background color"
      },
      {
        "type": "color",
        "messageKey": "FOREGROUND_COLOR",
        "defaultValue": "0xFFFFFF",
        "label": "Hours and Minutes arrow color"
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "More Settings"
      },
      {
        "type": "toggle",
        "messageKey": "SECONDS_TICK",
        "label": "Enable seconds arrow",
        "defaultValue": true
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  }
];