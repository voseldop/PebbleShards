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
        "label": "Primary background color"
      },
      {
        "type": "color",
        "messageKey": "FOREGROUND_COLOR",
        "defaultValue": "0xFFFFFF",
        "label": "Primary foreground color"
      },
      {
        "type": "color",
        "messageKey": "FOREGROUND2_COLOR",
        "defaultValue": "0xAAAAAA",
        "label": "Secondary foreground color"
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
      },
      {
        "type": "radiogroup",
        "messageKey": "TEMP_FORMAT",
        "label": "Temperature units",
        "defaultValue": "1",
        "options": [{"label" : "Metric", "value": "1"}, {"label" :"Imperial", "value" : "0"}]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Weather providers"
      },
      {
        "type": "radiogroup",
        "messageKey": "WEATHER_PROVIDER",
        "label": "Choose weather provider",
        "defaultValue": "0",
        "options": [{"label" : "<a href=\"http://openweathermap.org/terms\" target=\"_blank\"><img src=\"https://openweathermap.org/themes/openweathermap/assets/vendor/owm/img/logo_OpenWeatherMap_orange.svg\" width=\"134\" height=\"29\"/> </a>", "value": "0"}, {"label" :"<a href=\"https://www.wunderground.com/legal\" target=\"_blank\"><img src=\"https://icons.wxug.com/logos/PNG/wundergroundLogo_4c_rev_horz.png\" width=\"auto\" height=\"29\" /> </a>", "value" : "1"}, {"label" :"<a href=\"https://www.yahoo.com/?ilc=401\" target=\"_blank\"> <img src=\"https://poweredby.yahoo.com/white.png\" width=\"134\" height=\"29\"/> </a>", "value" : "2"}]
      },
      {
        "type": "input",
        "messageKey": "WEATHERAPI_KEY",
        "label": "User API Key",
        "attributes": {
          "limit": 31
        }
      }      
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  },
  {
    "type": "text",
    "defaultValue": "Please see list of used Open Source Software <a href=\"https://github.com/voseldop/shards/blob/master/NOTICE\">here</a>"
  }
];