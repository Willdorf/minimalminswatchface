(function() {
	loadOptions();
	submitHandler();
})();

function submitHandler() {
	var $submitButton = $('#submitButton');
	$submitButton.on('click', function() {
		console.log('Submit');

		var return_to = getQueryParam('return_to', 'pebblejs://close#');
		document.location = return_to + encodeURIComponent(JSON.stringify(getAndStoreConfigData()));
	});
}

var $submitButton = $('#submitButton');

$submitButton.on('click', function() {
	console.log('Submit');

	var return_to = getQueryParam('return_to', 'pebblejs://close#');
	document.location = return_to + encodeURIComponent(JSON.stringify(getAndStoreConfigData()));
});

var degreeOption = 0;
function tabClickHandler(value) {
	console.log(value);
	if (value == "Celsius") {
		degreeOption = 0;
	} else if (value == "Fahrenheit") {
		degreeOption = 1;
	}
}

function getAndStoreConfigData() {
	var $backgroundColorPicker = $('#backgroundColorPicker');

	var options = {
		backgroundColor : $backgroundColorPicker.val(),
		degreeOption : degreeOption
	};

	localStorage.willdorfminimalminsbackgroundColor = options.backgroundColor;
	localStorage.willdorfminimalminsdegreeOption = options.degreeOption;

	console.log('Got Options: ' + JSON.stringify(options));
	return options;
}

function loadOptions() {
	var $backgroundColorPicker = $('#backgroundColorPicker');

	if (localStorage.willdorfminimalminsbackgroundColor) {
		$backgroundColorPicker[0].value = localStorage.willdorfminimalminsbackgroundColor;

		//set the corresponding tab to active
		degreeOption = localStorage.willdorfminimalheartsdegreeOption;
		if (degreeOption == 0) {
			$('#Celsius').attr('class', 'tab-button active');
		} else {
			$('#Fahrenheit').attr('class', 'tab-button active');
		}
	} else {
		$('#Celsius').attr('class', 'tab-button active');
	}
}

function getQueryParam(variable, defaultValue) {
	var query = location.search.substring(1);
	var vars = query.split('&');
	for (var i = 0; i < vars.length; i++) {
		var pair = vars[i].split('=');
		if (pair[0] === variable) {
			return decodeURIComponent(pair[1]);
		}
	}
	return defaultValue || false;
}
