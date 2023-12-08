
function onProjectTitleClickGetProps( projectMenuEl, projectTitlePrefixId )
{
	if( projectMenuEl.style.display==='none' ) 
	{
		projectMenuEl.style.display = 'block';
		document.getElementById(projectTitlePrefixId).innerHTML = '&minus;&nbsp;';
		if( typeof(projectMenuEl.__myHasIfcLink) === 'undefined' ) 
		{
			let xhttpProps = new XMLHttpRequest();
			xhttpProps.onreadystatechange = function() 
			{
				if (xhttpProps.readyState == 4 ) 
				{
					if( xhttpProps.status == 200 ) 
					{
						let errorParsingProps = false;
						let props;	
						try {
							props = JSON.parse( xhttpProps.responseText );
						} catch(e) {
							errorParsingProps = true;
						}
						if( !errorParsingProps ) 
						{
							//console.log(props);
							if( 'project' in props && 'Name' in props.project ) {
								appendProjectMenuTitle(projectMenuEl, props.project.Name );
							} 
							appendProjectMenuLink( projectMenuEl, 'gantt', projectId, '__myHasGanttLink' );
							appendProjectMenuLink( projectMenuEl, 'dashboard', projectId, '__myHasHashboardLink');
							if( 'project' in props && 'WexbimPath' in props.project && props.project.WexbimPath.length > 0) {
								appendProjectMenuLink(projectMenuEl, 'ifc', projectId, '__myHasIfcLink' );
							} else {
								projectMenuEl.__myHasIfcLink = false;
							}
							if( 'project' in props && 'CurTime' in props.project ) {										
								appendProjectMenuInputLink( projectMenuEl, projectId, props.project.CurTime, '__myHasInputLink' );
							} else {
								projectMenuEl.__myInputIfcLink = false;
							}
						}
					}
				}
			} 
			xhttpProps.open( 'GET', '/.get_project_props?'+projectId, true );
			xhttpProps.send();

			let xhttpStructs = new XMLHttpRequest();
			xhttpProps.onreadystatechange = function() 
			{
				if (xhttpProps.readyState == 4 ) 
				{
					if( xhttpProps.status == 200 ) 
					{
						let errorParsingProps = false;
						let props;	
						try {
							props = JSON.parse( xhttpProps.responseText );
						} catch(e) {
							errorParsingProps = true;
						}
						if( !errorParsingProps ) 
						{
							console.log(props);
							if( 'project' in props && 'Name' in props.project ) {
								appendProjectMenuTitle(projectMenuEl, props.project.Name );
							} 
							appendProjectMenuLink( projectMenuEl, 'gantt', projectId, '__myHasGanttLink' );
							appendProjectMenuLink( projectMenuEl, 'dashboard', projectId, '__myHasHashboardLink');
							if( 'project' in props && 'WexbimPath' in props.project && props.project.WexbimPath.length > 0) {
								appendProjectMenuLink(projectMenuEl, 'ifc', projectId, '__myHasIfcLink' );
							} else {
								projectMenuEl.__myHasIfcLink = false;
							}
							if( 'project' in props && 'CurTime' in props.project ) {										
								appendProjectMenuInputLink( projectMenuEl, projectId, props.project.CurTime, '__myHasInputLink' );
							} else {
								projectMenuEl.__myInputIfcLink = false;
							}
						}
					}
				}
			} 
			xhttpStructs.open( 'GET', '/.get_gantt_structs?'+projectId, true );
			xhttpStructs.send();
		}
	} else {
		projectMenuEl.style.display = 'none';
		document.getElementById(projectTitlePrefixId).innerHTML = '&plus;&nbsp;';
	}
}


function onLoadContents()
{
		let data=null;
		let errorParsingStatusData = false;	
		let containerEl = document.getElementById('projects');
		try {
			data = JSON.parse(xhttp.responseText);
		} catch(e) {
			errorParsingStatusData = true;
		}
		if( errorParsingStatusData ) {
			containerEl.innerHTML = '...';			
		} else {
			if( !('Projects' in data) ) {
				containerEl.innerHTML = '...';			
			} else {
				if( data.Projects.length === 0 ) {
					containerEl.innerHTML = '...';			
				} else {
					containerEl.innerHTML = '';					
					for( let i = 0 ; i < data.Projects.length ; i++ ) {
						let projectTitle = data.Projects[i];
						let projectCode = data.Projects[i];
						let projectId = data.Projects[i]; 
					
						let projectContainerEl = document.createElement('div');
						containerEl.appendChild(projectContainerEl);
						projectContainerEl.className = 'project';
						if( i % 2 === 0 ) {
							projectContainerEl.style.backgroundColor = '#f0f7ff';
						}
						
						let projectTitleEl = document.createElement('div');
						projectContainerEl.appendChild(projectTitleEl);
						projectTitleEl.className = 'project-title';
						let projectTitlePrefixId = 'projectTitlePrefix' + i;
						projectTitleEl.innerHTML = `<span style='color:#7f7f7f;' id='${projectTitlePrefixId}'>&plus;&nbsp;</span>${projectTitle}`;

						let projectMenuEl = document.createElement('div');
						projectContainerEl.appendChild(projectMenuEl);
						projectMenuEl.className = 'project-menu';
						projectMenuEl.style.display = 'none';

						projectTitleEl.addEventListener( 'click', function(e) {
							onProjectTitleClickGetProps(projectMenuEl, projectTitlePrefixId);
						});
					}
				}
			}
		}
}

function loadContents() 
{	
	let xhttp = new XMLHttpRequest();
	xhttp.onreadystatechange = function() 
	{
		if (xhttp.readyState == 4 ) 
		{
			if( xhttp.status == 200 )
				onLoadContents();
      } 
	  }
	}
	xhttp.open( 'GET', '/.contents', true );
  xhttp.send();
}

/*
function loadContents() {
	let sectionIds = [ 'gantt', 'input', 'dashboard' ]
	let url = '/.contents';
	let sectionContainerEl = null;
	fetch( url ).then(data => data.json()).then( function(data) { 
		for( let i = 0 ; i < sectionIds.length ; i++ ) {
			let sectionId = sectionIds[i];
			if( !(sectionId in data) )
				continue;
			sectionContainerEl = document.getElementById(sectionId);
			if( !sectionContainerEl ) 
				continue;
		
			let html = '';
			let d = data[sectionId];
			if( d.length == 0 ) {
				sectionContainerEl.innerHTML = '...';			
				continue;
			}
			sectionContainerEl.innerHTML = '';
			for( let j = 0 ; j < d.length ; j++ ) {
				let aEl = document.createElement('a');
				aEl.appendChild( document.createTextNode(d[j].title) );
				let href = `/${sectionId}/index.html?${d[j].id}`;
				aEl.href = href;
				aEl.onclick = function(e) { e.preventDefault(); checkAuthorizedAndProceed( href ); };
				sectionContainerEl.appendChild(aEl);
				sectionContainerEl.appendChild(document.createElement('br'));
			}
			//elem.innerHTML=html; 
			
		}
	}).catch( function(e) { console.log(e); } );
}
*/

function checkAvailabilityAndProceed(url, clearCookie=false) {	
	displayErrorMessage('connectionError', false);
	checkServer( 2, function(available, notUsed) {
		if( !available ) {
			displayErrorMessage('connectionError');
		} else {
			if( clearCookie ) {
				deleteCookie('user');
				deleteCookie('sess_id');
			}
			window.location.href = url;
		}
	});	
}


function checkAuthorizedAndProceed(url) {	
	displayErrorMessage('connectionError', false);
	checkServer( 2, function(available, authorized) {
		if( !available ) {
			displayErrorMessage('connectionError');
		} else {
			if( !authorized )
				window.location.href = '/';
			else
				window.open(url);
		}
	});	
}

window.addEventListener('load', function() { 
	let logoutEl = document.getElementById('a-logout');
	if( logoutEl ) {
		logoutEl.onclick = function(e) { e.preventDefault(); checkAvailabilityAndProceed('/.logout', true); };		
	}
});


function secondsIntoSpiderDateString( seconds, props = { addSeconds:0, dateFormat:'DMY' } ) {
	if( typeof(seconds) === 'undefined' || seconds === null || seconds === '' ) {
		return '';
	}

	let spiderDateString = '';
	let date;
	try {
		date = new Date( parseInt(seconds) * 1000 + (('addSeconds' in props) ? props.addSeconds*1000 : 0) );
	} catch(e) {
		return '';
	}

	let year = date.getUTCFullYear(); 
	let month = (date.getUTCMonth()+1);
	if( month < 10 ) {
		month = "0" + month;
	}
	let day = date.getUTCDate();
	if( day < 10 ) {
		day = "0" + day;
	}
	let dateFormat = ('dateFormat' in props) ? props.dateFormat : 'DMY';
	let dateDelim = ('dateDelim' in props) ? props.dateDelim : '.';
	if( dateFormat === 'DMY' ) {
		spiderDateString = day + dateDelim + month + dateDelim + year; 
	} else {
		spiderDateString = month + dateDelim + day + dateDelim + year;		 
	}
	return( spiderDateString ); 
}


function parseDate( dateString, props = { dateFormat:'DMY' } ) {
	if( typeof(dateString) === 'undefined' ) {
		return null;
	}
	if( dateString == null ) {
		return null;
	}
	let date = null;
	let y=null, m=null, d=null, hr=null, mn=null;
	let parsedFull = dateString.match( /([0-9]+)[\.\/\-\:]([0-9]+)[\.\/\-\:]([0-9]+)[ T]+([0-9]+)[\:\.\-\/]([0-9]+)/ );
	if( parsedFull !== null ) {
		if( parsedFull.length == 6 ) {
			y = parsedFull[3];
			if( 'dateFormat' in props && props.dateFormat === 'MDY' ) {
				m = parsedFull[1];
				d = parsedFull[2];								
			} else {
				d = parsedFull[1];
				m = parsedFull[2];				
			}
			hr = parsedFull[4];
			mn = parsedFull[5];
			date = new Date( Date.UTC(y, m-1, d, hr, mn, 0, 0) );
		}
	} else {
		let parsedShort = dateString.match( /([0-9]+)[\.\/\-\:]([0-9]+)[\.\/\-\:]([0-9]+)/ );
		if( parsedShort !== null ) {
			if( parsedShort.length == 4 ) {
				y = parsedShort[3];
				if( 'dateFormat' in props && props.dateFormat === 'MDY' ) {
					m = parsedShort[1];
					d = parsedShort[2];								
				} else {
					d = parsedShort[1];
					m = parsedShort[2];				
				}
				hr = 0;
				mn = 0;
				date = new Date(Date.UTC(y, m-1, d, hr, mn, 0, 0, 0, 0));
			}
		}
	}
	if( date === null ) {
		return null;
	}
	let timeInSeconds = date.getTime();
	return( { 'date':date, 'timeInSeconds':timeInSeconds/1000 } ); 
}


window.addEventListener( 'load', function() { loadContents(); } );

window.addEventListener('load', function() { handleLang(); });

</script>

</body>

</html>