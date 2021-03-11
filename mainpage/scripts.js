var _langs = [ 'en', 'ru' ];

var _texts = { 
	'text-lang': { 'en': 'EN', 'ru': 'РУ' },
	'text-logout': { 'en': 'exit', 'ru': 'выход' },
	'text-not-logged-in': { 'en': 'You are not logged in', 'ru': 'Вы не авторизованы' },
	'text-user': { 'en': 'User', 'ru': 'Имя пользователя' },
	'text-pass': { 'en': 'Password', 'ru': 'Пароль' },
	'text-login-page-title': { 'en': 'Web SP Login', 'ru': 'Вход в Web SP' },
	'text-login-button': { 'en': 'Login', 'ru': 'Вход' },
	'text-index-page-title': { 'en': 'Web SP Main Menu', 'ru': 'Главное меню Web SP' },
	'text-gantt-charts': { 'en': 'Gantt Charts', 'ru': 'Диаграммы Гантта' },
	'text-projects': { 'en': 'Projects', 'ru': 'Проекты' },
	'text-input-tables': { 'en': 'Input Tables', 'ru': 'Учет' },
	'text-dashboards': { 'en': 'Dashboards', 'ru': 'Дэшборды' },
	'text-connection-error': { 'en': 'Connection with your server is lost. Please try again later...', 
		'ru':'Связь с сервером потеряна. Пожалуйста попытайтесь позже...' },
	'text-credentials-error': { 'en': 'Login and/or password is incorrect. Please, try again...', 
		'ru':'Логин и/или пароль указан(ы) неверное. Пожалуйста попытайтесь снова...' },
	'text-user-name-error': { 'en': '"User" field contains an invalid value. Please, try again...', 
		'ru':'Поле "Пользователь" содержит недопустимое значение. Пожалуйста попытайтесь снова...' },
	'text-pass-error': { 'en': '"Password" field contains an invalid value. Please, try again...', 
		'ru':'Поле "Пользователь" содержит недопустимое значение. Пожалуйста попытайтесь снова...' },
};
                                                                  

function handleLangTexts( lang ) {
	for( let key in _texts ) {
		let el = document.getElementById(key);
		if( !el ) {
			continue;
		}
		if( !(lang in _texts[key]) ) {
			continue;
		}
		el.innerHTML = _texts[key][lang];
	}
	let userName = getCookie('user');	
	if( userName !== null ) {
		let el = document.getElementById('text-user-name');
		if( el )
			el.innerHTML = userName;
	}	
}

function handleLang( langId ) {
	let langEl = document.getElementById('header-lang');
	if( !langEl ) {
		return;
	}

	let lang = null;
	let langIndex = -1;

	// Reading lang from uri
	let ss = window.location.search;
	if( ss.length > 1 ) {
		lang = ss.substr(1).trim().toLowerCase();		
		langIndex = _langs.indexOf(lang);
	}
	// If no lang found, reading from cookie
	if( langIndex === -1 ) {
		lang = getCookie( 'lang' );
		langIndex = _langs.indexOf(lang);
	}
	if( langIndex === -1 ) {
		langIndex = 0;
		lang = _langs[langIndex];
	}	
	//langEl.innerHTML = lang;	
	handleLangTexts( lang );
	langEl.dataset.langindex = langIndex;
	setCookie( 'lang', lang );

	langEl.onclick = function(e) {
		let index = parseInt( this.dataset.langindex );
		if( index < 0 || index > _langs.length-1 ) {
			return;
		}
		if( index < _langs.length-1 ) {
			index += 1;
		} else {
			index = 0;
		}
		let lang = _langs[index];
		//langEl.innerHTML = lang;
		handleLangTexts( lang );
		this.dataset.langindex = index;
		setCookie( 'lang', _langs[index] );
	};
}


function displayLoader( loaderEl, status ) {
	if( loaderEl ) {
		if( status ) {
			loaderEl.style.display = 'inline-block';
		} else {
			loaderEl.style.display = 'none';
		}
	}
}

function displayErrorMessage( errorMessageId, display=true ) {
	let id = document.getElementById(errorMessageId);
	if( id ) {
		if( display ) {
			id.style.display = 'inline-block';
		} else {
			id.style.display = 'none';
		}
	}	
}


function setCookie( cname, cvalue, exminutes=null ) {
	if( exminutes === null ) {
		document.cookie = `${cname}=${cvalue}; path=/`;
	}
	else {
		let d = new Date();
		d.setTime(d.getTime() + (exminutes*60*1000));
		document.cookie = `${cname}=${cvalue}; expires=${d.toUTCString()}; path=/`;
	}
}

function deleteCookie( cname ) {
	document.cookie = `${cname}=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/`;
}

/*''*/

function getCookie( cname, type='string' ) {
	let name = cname + "=";
	let decodedCookie = decodeURIComponent(document.cookie);
	let ca = decodedCookie.split(';');
	for( let i = 0 ; i < ca.length ; i++ ) {
		let c = ca[i];
		while( c.charAt(0) == ' ' ) {
			c = c.substring(1);
		}
		if( c.indexOf(name) == 0 ) {
			let value = c.substring(name.length, c.length);
			if( type == 'string' ) {
				return value;
			}
			if( type == 'int' ) {
				let intValue = parseInt(value);
				if( !isNaN(intValue) ) {
					return intValue;
				}
			}
			if( type == 'float' ) {
				let floatValue = parseFloat(value);
				if( !isNaN(floatValue) ) {
					return floatValue;
				}
			}
			return null;
		}
	}
	return null;
}
