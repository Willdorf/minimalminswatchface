Pebble.addEventListener('showConfiguration', function() {
	var url = 'http://armstrongwill.com/pebbleconfigurationpages/minimalmins/index.html';

	console.log('Showing configuration page: ' + url);

	Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
	var configData = JSON.parse(decodeURIComponent(e.response));

	console.log('Configuration page returned: ' + JSON.stringify(configData));

	if (configData.backgroundColor) {
		Pebble.sendAppMessage({
			backgroundColor : parseInt(configData.backgroundColor, 16),
			degreeOption : parseInt(configData.degreeOption)
		}, function() {
			console.log("send successful!");
		}, function() {
			console.log("send failed!");
		});
	}

	getWeather();
});

//Listen for when AppMessage is received
Pebble.addEventListener('appmessage',
		function(e) {
			console.log('AppMessage receieved!');
			getWeather();
		}
	);

var xhrRequest = function(url, type, callback) {
	var xhr = new XMLHttpRequest();
	xhr.onload = function() {
		callback(this.responseText);
	}
	xhr.open(type, url);
	xhr.send();
}

function locationSuccess(pos) {
	var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' +
		pos.coords.latitude + '&lon=' + pos.coords.longitude + '&APPID='+APPID;
	xhrRequest(url, 'Get',
		function(responseText) {
			//responseText contains a JSON object with weather info
			console.log(JSON.parse(responseText));
			var json = JSON.parse(responseText);

			//Temperature is in Kelvin
			var temperature = Math.round(json.main.temp);
			console.log('Temperature is ' + temperature);

			//Conditions
			var conditions = json.weather[0].main;
			console.log('Conditions are ' + conditions);

			var dictionary = {
				'KEY_TEMPERATURE' : temperature,
				'KEY_CONDITIONS' : conditions
			};

			Pebble.sendAppMessage(dictionary,
				function(e) {
					console.log('Weather info sent to Pebble successfully!');
				},
				function(e) {
					console.log('Error sending weather info to Pebble!');
				}
				);
		}
	);
}

function locationError(err) {
	console.log('Error requesting location!');
}

function getWeather() {
	navigator.geolocation.getCurrentPosition(
		locationSuccess,
		locationError,
		{timeout: 15000, maximumAge: 6000}
		);
}

Pebble.addEventListener('ready', function(e) {
	console.log('PebbleKit JS ready!');

	//get the initial weather
	getWeather();
});	
