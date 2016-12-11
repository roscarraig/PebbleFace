// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
    getWeather();
    stockCheck('NYSE:WDAY');
  }                     
);

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function stockCheck(symbol) {
  var url = "http://finance.google.com/finance/info?q="+symbol;
  
  xhrRequest(url, 'GET',
            function(response) {
              console.log(response.slice(3));
              var json = JSON.parse(response.slice(3));
              var price = json[0].l;
              console.log(price);
              var dictionary = {
                "KEY_SHAREPRICE" : price
              };
              Pebble.sendAppMessage(dictionary,
                function(e) {
                  console.log("Share info sent to Pebble successfully!");
                },
                function(e) {
                  console.log("Error sending share info to Pebble!");
                }
              );
            });
}

function locationSuccess(pos) {
  // Construct URL
  var url = "http://api.openweathermap.org/data/2.5/weather?lat=" +
      pos.coords.latitude + "&lon=" + pos.coords.longitude +
      "&APPID=39c807b09d4687f15ea4c37906fb3f7d";
  console.log("URL is" + url);

  // Send request to OpenWeatherMap
  xhrRequest(url, 'GET', 
    function(responseText) {
      console.log(responseText);
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);

      // Temperature in Kelvin requires adjustment
      var temperature = Math.round(json.main.temp - 273.15);
      console.log("Temperature is " + temperature);

      // Conditions
      var conditions = json.weather[0].main + ' ' + json.name;      
      console.log("Conditions are " + conditions);


  // Assemble dictionary using our keys
var dictionary = {
  "KEY_TEMPERATURE": temperature,
  "KEY_CONDITIONS": conditions
};


// Send to Pebble
Pebble.sendAppMessage(dictionary,
  function(e) { console.log("Weather info sent to Pebble successfully!");},
  function(e) { console.log("Error sending weather info to Pebble!");}
);
    }      
  );

}


function locationError(err) {
  console.log("Error requesting location!");
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

stockCheck('NYSE:WDAY');
// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
   function() { 
     console.log("PebbleKit JS ready!");
     Pebble.sendAppMessage({'JSReady': 1});

    // Get the initial weather
    getWeather();
    stockCheck('NYSE:WDAY');
  }
);