function loadQueries(){
    let tb = document.getElementsByTagName('tbody')[0];
    while(tb.firstChild)
    {
        tb.removeChild(tb.firstChild);
    }
    
    let http = new XMLHttpRequest();
    http.open("GET", "/querylog.json");

    http.onreadystatechange = function() {
        if (http.readyState == 4){
            if (http.status == 200)
            {
                let log = JSON.parse(http.response);
                log.pop(); // last element is always empty object
                console.log(log);
                
                var size = document.getElementById('sizeselect').value;
                for(let i = 0; i < Math.min(log.length, size); i++)
                {
                    let entry = log[i];
                    let row = tb.insertRow();
                    row.insertCell().innerHTML = entry.time;
                    row.insertCell().innerHTML = entry.domain;
                    row.insertCell().innerHTML = entry.client;

                    let button = document.createElement('button');
                    if (entry.blocked == true){
                        button.className = 'altbtn';
                        button.innerHTML = "&check;"
                        row.style.color = "#FF0000";
                    }
                    else{
                        button.innerHTML = "&times;"
                        button.onclick = function(){updateBlacklist("PUT", entry.domain)};
                    }
    
                    row.insertCell().appendChild(button);
                }
            }
            else
            {
                let err = document.getElementById('error');
                console.log("Error loading queries")
                err.innerHTML = http.responseText;
                err.style.visibility = 'visible'
            }
        }
    }
    http.send();
}

function loadBlacklist(){
    let err = document.getElementById('error');
    err.style.visibility = 'hidden';

    let sumbit = document.getElementById('urlinput');
    sumbit.disabled = true

    const http = new XMLHttpRequest();
    http.open("GET", '/blacklist.txt');

    http.onreadystatechange = function() {
        if (http.readyState == 4){
            if (http.status == 200){
                sumbit.disabled = false
                list = http.response.split('\n').reverse();
                console.log(list);
                
                // Remove old list
                let tb = document.getElementsByTagName('tbody')[0];
                while(tb.firstChild)
                {
                    tb.removeChild(tb.firstChild);
                }

                // Add new list
                for(let i = 1; i < list.length; i++){ // first element is empty string
                    let row = tb.insertRow();
                    let domain = row.insertCell();
                    let deleteButton = row.insertCell();

                    domain.innerHTML = list[i];

                    let button = document.createElement('button');
                    button.innerHTML = '&times;';
                    button.onclick = function(){updateBlacklist("DELETE", list[i])};
                    deleteButton.appendChild(button)
                }
            }
            else {
                err.innerHTML = http.responseText;
                err.style.visibility = 'visible';
            }
        }
    };
    http.send();
}

function addToBlacklist(){
    let urlinput = document.getElementById('urlinput');
    console.log("adding " + urlinput.value)
    updateBlacklist("PUT", urlinput.value)
}

function updateBlacklist(action, hostname){
    let err = document.getElementById('error');
    let http = new XMLHttpRequest();
    http.open(action, "/blacklist/" + hostname);

    http.onreadystatechange = function(){
        if (http.readyState == 4){
            console.log(http.status)
            if (http.status == 200){
                err.style.visibility = 'hidden';
                location.href = "/blacklist"
            }
            else{
                
                err.innerHTML = http.responseText;
                err.style.visibility = 'visible';
            }
        }
    };
    http.send();
}

function loadSettings(){
    let err = document.getElementById('error');
    // err.style.visibility = 'hidden'

    const http = new XMLHttpRequest();
    http.open("GET", '/settings.json');

    http.onreadystatechange = function() {
        if (http.readyState == 4){
            if (http.status == 200){
                let settings = JSON.parse(http.response);

                var button = document.getElementById("blockStatus")
                var version = document.getElementById("version");
                var updatesrv = document.getElementsByName("updatesrv")[0];
                var dnssrv = document.getElementsByName("dnssrv")[0];
                var url = document.getElementsByName("url")[0];
                var ip = document.getElementsByName("ip")[0];
                var update_available = document.getElementById("update_available");
                var updateButton = document.getElementById("updateButton");

                if(settings.ip){
                    ip.value = settings.ip;
                    ip.disabled = false
                }

                if(settings.url){
                    url.value = settings.url;
                    url.disabled = false
                }

                if(settings.update_srv){
                    updatesrv.value = settings.update_srv;
                    updatesrv.disabled = false
                }

                if(settings.version){
                    version.innerHTML = settings.version;
                }

                for(let j = 0; j < dnssrv.options.length; j++){
                    if (dnssrv.options[j].value == settings.dns_srv){
                        dnssrv.selectedIndex = j;
                        dnssrv.disabled = false;
                    }
                }

                if( settings.blocking ){
                    button.innerHTML = 'Blocking On';
                    button.className = "";
                    button.disabled = false;
                    button.style.visibility = "visible";
                }else if( !settings.blocking ){
                    button.className = "altbtn";
                    button.innerHTML = 'Blocking Off';
                    button.disabled = false;
                    button.style.visibility = "visible";
                }
                
                if( settings.update_available ){
                    update_available.style.visibility = 'visible';
                }else{
                    update_available.style.visibility = 'hidden';
                }
            }
            else {
                err.innerHTML = http.responseText;
                err.style.visibility = 'visible';
            }
        }
    };
    http.send();
}

function toggleBlock()
{
    let err = document.getElementById('error');
    const http = new XMLHttpRequest();
    http.open("POST", '/toggleblock');

    http.onreadystatechange = function() {
        if (http.readyState == 4){
            if (http.status == 200){
                console.log(http.responseText)
                var button = document.getElementById("blockStatus")

                if( http.responseText == "false")
                {
                    button.className = "altbtn";
                    button.innerHTML = 'Blocking Off';
                }
                else if( http.responseText == "true" )
                {
                    button.innerHTML = 'Blocking On';
                    button.className = "";
                }
            }
            else {
                err.innerHTML = http.responseText;
                err.style.visibility = 'visible';
            }
        }
    };
    http.send();
}


function closeModal()
{
    let updateModal = document.getElementById("updateModal")
    updateModal.style.display = "none"
}


function updateFirmware()
{
    let updateModal = document.getElementById("updateModal");
    let updatestatus = document.getElementById('updatestatus')
    let closebutton = document.getElementById('closemodal')
    updateModal.style.display = "block";

    let err = document.getElementById('error');
    const http = new XMLHttpRequest();
    http.open("POST", '/updatefirmware');

    http.onreadystatechange = function() {
        if (http.readyState == 4){
            if (http.status == 200){
                updatestatus.innerHTML = "Restarting..."
                closebutton.style.visibility = "hidden";
                restart();
            }
            else {
                err.innerHTML = http.responseText;
                err.style.visibility = 'visible';
            }
        }
    };
    http.send();
}

function restart()
{
    const http = new XMLHttpRequest();
    http.open("POST", '/restart');

    http.onreadystatechange = function() {
        if (http.readyState == 4){
            if (http.status == 200){
                window.setTimeout(restartFinished, 2000);
            }
            else {
                closeModal()
                err.innerHTML = http.responseText;
                err.style.visibility = 'visible';
            }
        }
    };
    http.send();
}

function restartFinished()
{
    const http = new XMLHttpRequest();
    http.open("GET", '/settings.json');

    http.onreadystatechange = function() {
        if (http.readyState == 4){
            if (http.status == 200){
                location.href = "/settings";
            }
        }
    };
    http.send();
}