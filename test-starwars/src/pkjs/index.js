var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () { callback(this.responseText); };
  xhr.open(type, url);
  xhr.send();
};

// Map WMO weather codes to Star Wars planets
function weatherCodeToPlanet(code) {
  if (code === 0) return 'Tatooine';        // Clear sky — desert
  if (code <= 3) return 'Bespin';            // Cloudy — cloud city
  if (code <= 48) return 'Dagobah';          // Fog — swamp planet
  if (code <= 57) return 'Kamino';           // Drizzle — rain planet
  if (code <= 67) return 'Kamino';           // Rain — rain planet
  if (code <= 77) return 'Hoth';             // Snow — ice planet
  if (code <= 86) return 'Hoth';             // Snow showers
  if (code <= 99) return 'Mustafar';         // Thunderstorm — lava planet
  return 'Coruscant';
}

function locationSuccess(pos) {
  var url = 'https://api.open-meteo.com/v1/forecast?' +
      'latitude=' + pos.coords.latitude +
      '&longitude=' + pos.coords.longitude +
      '&current=temperature_2m,weather_code';

  xhrRequest(url, 'GET', function(responseText) {
    var json = JSON.parse(responseText);
    var temperature = Math.round(json.current.temperature_2m);
    var conditions = weatherCodeToPlanet(json.current.weather_code);

    Pebble.sendAppMessage(
      { 'TEMPERATURE': temperature, 'CONDITIONS': conditions },
      function(e) { console.log('Weather sent!'); },
      function(e) { console.log('Weather send failed!'); }
    );
  });
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess, locationError,
    { timeout: 15000, maximumAge: 60000 }
  );
}

Pebble.addEventListener('ready', function(e) {
  console.log('PebbleKit JS ready!');
  getWeather();
});

Pebble.addEventListener('appmessage', function(e) {
  if (e.payload['REQUEST_WEATHER']) { getWeather(); }
});
