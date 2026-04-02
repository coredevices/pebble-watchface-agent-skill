/**
 * PebbleKit JS Weather Template
 *
 * Fetches weather data from Open-Meteo API (free, no API key needed)
 * and sends temperature + conditions to the watch via AppMessage.
 *
 * Requires package.json to have:
 *   "capabilities": ["location"],
 *   "messageKeys": ["TEMPERATURE", "CONDITIONS", "REQUEST_WEATHER"],
 *   "enableMultiJS": true
 *
 * Place this file at: src/pkjs/index.js
 */

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

/**
 * Convert WMO weather codes to short human-readable strings.
 * See: https://open-meteo.com/en/docs (WMO Weather interpretation codes)
 */
function weatherCodeToCondition(code) {
  if (code === 0) return 'Clear';
  if (code <= 3) return 'Cloudy';
  if (code <= 48) return 'Fog';
  if (code <= 55) return 'Drizzle';
  if (code <= 57) return 'Fz. Drizzle';
  if (code <= 65) return 'Rain';
  if (code <= 67) return 'Fz. Rain';
  if (code <= 75) return 'Snow';
  if (code <= 77) return 'Snow Grains';
  if (code <= 82) return 'Showers';
  if (code <= 86) return 'Snow Shwrs';
  if (code === 95) return 'T-Storm';
  if (code <= 99) return 'T-Storm';
  return 'Unknown';
}

function locationSuccess(pos) {
  // Open-Meteo: free weather API, no key required
  var url = 'https://api.open-meteo.com/v1/forecast?' +
      'latitude=' + pos.coords.latitude +
      '&longitude=' + pos.coords.longitude +
      '&current=temperature_2m,weather_code';

  xhrRequest(url, 'GET', function(responseText) {
    var json = JSON.parse(responseText);
    var temperature = Math.round(json.current.temperature_2m);
    var conditions = weatherCodeToCondition(json.current.weather_code);

    var dictionary = {
      'TEMPERATURE': temperature,
      'CONDITIONS': conditions
    };

    Pebble.sendAppMessage(dictionary,
      function(e) { console.log('Weather info sent to Pebble successfully!'); },
      function(e) { console.log('Error sending weather info to Pebble!'); }
    );
  });
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    { timeout: 15000, maximumAge: 60000 }
  );
}

// Fetch weather when JS runtime is ready
Pebble.addEventListener('ready', function(e) {
  console.log('PebbleKit JS ready!');
  getWeather();
});

// Handle weather refresh requests from the watch
Pebble.addEventListener('appmessage', function(e) {
  console.log('AppMessage received!');
  if (e.payload['REQUEST_WEATHER']) {
    getWeather();
  }
});
