
var _checkServerAvailabilityTimeout = -1;
var _checkServerAvailabilityInProgress = false;

function clearServerAvailabilityTimeout() {
	if( _checkServerAvailabilityTimeout !== -1 ) {
		clearTimeout( _checkServerAvailabilityTimeout );
		_checkServerAvailabilityTimeout = -1;
	}
}


// check: 1: availability, 2: authorized
function checkServer( check=1, callback=null, scheduleNext=null) {
	if( _checkServerAvailabilityInProgress !== false ) {
		return;
	}
	_checkServerAvailabilityInProgress = true;

	let xmlhttp = new XMLHttpRequest();
	xmlhttp.onreadystatechange = function() {
	    if( this.readyState == 4 ) {
			let availabe;
			let authorized=null;
	    	if( this.status == 200 ) {
				available = true;
				if( check == 2 ) { 	// Cheking authorized?
					if( this.responseText == '1' ) {
						authorized = true;
					} else {			
						authorized = false;
					}
				}
			} else {
				available = false;
			}
			
			if( callback !== null ) {
				callback(available, authorized);
			}
			
			if( _checkServerAvailabilityTimeout !== -1 ) {
				_checkServerAvailabilityTimeout = -1;
			}
			if( scheduleNext !== null ) {
				_checkServerAvailabilityTimeout = setTimeout( 
					function() { 
						checkServerAvailability( callback, scheduleNext, callbackParameters); 
					}, 
					scheduleNext );
			}

			_checkServerAvailabilityInProgress = false;
	    }
	}
	if( check == 1 ) { // Checking availabitity
		xmlhttp.open("GET", '/.check_connection', true); 	// Checking connection...
	} else {
		xmlhttp.open("GET", '/.check_authorized', true); 	// Checking authorization...
	}
    xmlhttp.send();
}
