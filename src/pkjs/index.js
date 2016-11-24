
var weatherKeys = require('./keys');
var user_key='';
// Import the Clay package
var Clay = require('pebble-clay');
// Load our Clay configuration file
var clayConfig = require('./config');

// Initialize Clay
var clay = new Clay(clayConfig);

function AbstractWeather () {}
AbstractWeather.prototype.locationSuccess = function (pos) {
  var url = this.getURL(pos);
  
  console.log('Request is ' + url);
  var xhr = new XMLHttpRequest();
  var ctx = this;
  xhr.onload = function () {
    ctx.processResponse(this.responseText);
  };
  xhr.open('GET', url);
  xhr.send();
};

AbstractWeather.prototype.locationError = function (err) {
  console.log('Error requesting location!');
};

AbstractWeather.prototype.reportWeather = function (temperature, conditionsCode) {
  // Assemble dictionary using our keys
  var dictionary = {
    'TEMPERATURE': temperature,
    'CONDITIONS': conditionsCode
  };

  // Send to Pebble
  Pebble.sendAppMessage(dictionary,
                        function(e) {
                          console.log('Weather info sent to Pebble successfully!');
                        },
                        function(e) {
                          console.log('Error sending weather info to Pebble!');
                        }
                       );
};

AbstractWeather.prototype.getWeather = function ()
{
  navigator.geolocation.getCurrentPosition(
    this.locationSuccess.bind(this),
    this.locationError.bind(this),
    {timeout: 15000, maximumAge: 60000}
    );
};


function OpenWeather() {}
OpenWeather.prototype = Object.create(AbstractWeather.prototype);
OpenWeather.prototype.constructor = OpenWeather;

OpenWeather.prototype.getWeatherCode = function (icon)  {
  var code = {800: 'N', 801: 'C', 802: 'S', 803: 'S', 804: 'S', 
              600: 'a', 601: 'a', 602: 'a', 611: 'a', 612: 'a', 615: 'a', 616: 'a', 620: 'a', 621: 'a', 622: 'a',
              500: 'L', 501: 'L', 502: 'L', 503: 'L', 504: 'L', 511: 'b', 520: 'b', 521: 'b', 522: 'b', 531: 'b', 
              200: 'V', 201: 'V', 202: 'V', 210: 'V', 211: 'V', 212: 'V', 221: 'V', 230: 'V', 231: 'V', 232: 'V',
              701: '!', 711: '!', 721: '!', 731: '!', 741: '!', 751: '!', 761: '!', 762: '!', 771: '!', 781: '!'};
  return code[icon];
};

OpenWeather.prototype.getURL = function (pos)  {
  var apiKey=weatherKeys.owm_key;
  if (user_key!==undefined && user_key!==null && user_key.length>0)
  {
    apiKey = user_key;
  }
  return 'http://api.openweathermap.org/data/2.5/weather?lat=' +
      pos.coords.latitude + '&lon=' + pos.coords.longitude + '&appid=' + apiKey + "&units=metric";
};
  
OpenWeather.prototype.processResponse = function(responseText) {
  // responseText contains a JSON object with weather info
  var json = JSON.parse(responseText);

  // Temperature in Kelvin requires adjustment
  var temperature = Math.round(json.main.temp);
  console.log('Temperature is ' + temperature);

  // Conditions
  var conditions = json.weather[0].id;      
  console.log('Conditions are ' + conditions);
  var code = this.getWeatherCode(conditions);

  this.reportWeather(temperature, code);
};  


function Yahoo () {}
Yahoo.prototype = Object.create(AbstractWeather.prototype);
Yahoo.prototype.constructor = Yahoo;

Yahoo.prototype.getWeatherCode = function (icon) {
  var code = [0, 0, 0, 'V', 'V', 'a', 'a', 'a', 'b', 'b', 'b', 'L', 'L', 'a', 'a', 'a', 'a', 0, 'a', '!', '!', '!', '!', 0, 0, 0, 'S', 'S', 'S', 'S', 'S', 'N', 'N', 'N', 'N', 0, 0, 'V', 'V', 'V', 'L', 'a', 'a', 'a', 'S', 'V', 'a', 'V', 0];
  return code[icon];
};
  
Yahoo.prototype.processResponse = function(responseText) {
  // responseText contains a JSON object with weather info
  var json = JSON.parse(responseText);

  // Temperature in Kelvin requires adjustment
  var temperature = Math.round(json.query.results.channel.item.condition.temp);
  console.log('Temperature is ' + temperature);

  // Conditions
  var conditions = json.query.results.channel.item.condition.code;      
  console.log('Conditions are ' + conditions);
  var code = this.getWeatherCode(conditions);
  
  this.reportWeather(temperature, code);
};

Yahoo.prototype.getURL = function (pos)  {
  var locationQuery = encodeURI("select item.condition.temp, item.condition.code from weather.forecast where woeid in (select woeid from geo.places where text='("+ pos.coords.latitude + ',' + pos.coords.longitude +")') and u='c'");
  var url = "http://query.yahooapis.com/v1/public/yql?q=" + locationQuery + "&format=json";
  return url;
};


function WeatherUndeground() {}
WeatherUndeground.prototype = Object.create(AbstractWeather.prototype);
WeatherUndeground.prototype.constructor = WeatherUndeground;

WeatherUndeground.prototype.getWeatherCode = function (icon) {
  switch (icon)
  {
    case "clear":
    case "sunny":
      return 'N';
    case "cloudy":
    case "mostlycloudy":
    case "partlycloudy":
      return 'S';

    case "partlysunny":
    case "partlycloudy":
    case "mostlysunny":      
      return 'C';
    case "tstorms":
    case "chancetstorms":
      return 'V';

    case "chancesnow":
    case "chanceflurries":
    case "flurries":
    case "snow":
      return 'a'; 

    case "chancerain":
    case "rain":
      return 'L';
    case "hazy":
    case "fog":
      return '!';

    case "chancesleet":
    case "sleet":
      return 'b';     
    default:        
      return 0;
  }
};

WeatherUndeground.prototype.processResponse = function(responseText) {
  // responseText contains a JSON object with weather info
  var json = JSON.parse(responseText);

  // Temperature in Kelvin requires adjustment
  var temperature = Math.round(json.current_observation.temp_c);
  console.log('Temperature is ' + temperature);

  // Conditions
  var conditions = json.current_observation.icon;      
  console.log('Conditions are ' + conditions);

  this.reportWeather(temperature, this.getWeatherCode(conditions));  
};

WeatherUndeground.prototype.getURL = function (pos)  {
  var apiKey=weatherKeys.wunderground_key;
  if (user_key!==undefined && user_key!==null && user_key.length>0)
  {
    apiKey = user_key;
  }
  return 'http://api.wunderground.com/api/'+ apiKey + '/conditions/q/' +
      pos.coords.latitude + ',' + pos.coords.longitude + '.json';
};


console.log("My app has started - Doing stuff...");

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log('PebbleKit JS ready!');
  }
);
// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {    
    var providers = [new OpenWeather(), new WeatherUndeground(), new Yahoo()];
    
    // Get the dictionary from the message
    var dict = e.payload;    
    user_key = dict.WEATHERAPI_KEY;
    
    console.log('Got message: ' + JSON.stringify(dict));
    
    providers[dict.WEATHER_PROVIDER].getWeather();
  }                     
);
