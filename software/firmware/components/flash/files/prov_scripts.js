function get_wifi_json(){
    let http = new XMLHttpRequest();
    http.open("GET", '/wifi.json');

    http.onreadystatechange = function() {
        if (http.readyState == 4){
            let scanning = document.getElementById("scanning");
            scanning.style.visibility="hidden";

            if (http.status == 200){
                // Remove current options
                var ssid_select = document.getElementById("ssid");
                while(ssid_select.firstChild)
                {
                    ssid_select.removeChild(ssid_select.firstChild);
                }

                // parse and sort response
                var response = JSON.parse(http.response);
                response.sort((a,b) => (a.rssi > b.rssi) ? -1 : 1);

                // add new ssids
                for(let i = 0; i < response.length; i++)
                {
                    let option = document.createElement("option");
                    option.value = response[i].ssid;
                    option.innerHTML = response[i].ssid;
                    ssid_select.appendChild(option)
                }
            }
            else{
                err.innerHTML = http.statusText + ", " + http.responseText;
                err.style.visibility = 'visible'
                scanning.style.visibility="hidden";
            }
        } 
    }

    http.send();
}

function submit_auth_data(){
    // Hide previous error
    let err = document.getElementById("error");
    err.style.visibility = 'hidden';

    // show spinner
    let scanning = document.getElementById("scanning");
    scanning.style.visibility="visible";
    
    // get ssid and password
    let ssid_select = document.getElementById('ssid');
    let password_input = document.getElementById('pw');
    let auth_data = { ssid: ssid_select.value, pass: password_input.value };

    // send data
    const http = new XMLHttpRequest();
    http.open("POST", '/submitauth');
    http.send(JSON.stringify(auth_data));

    let modal = document.getElementById('modal')
    modal.style.display = 'block';

    http.onreadystatechange = function() {
        if (http.readyState == 4)
        {
            if (http.status == 200){
                if( http.responseText == 'connected')
                { 
                    window.location.href = '/connected';
                }
                else if( http.responseText == 'could not connect')
                {
                    err.innerHTML = "Could not connect to " + ssid_select.value;
                    err.style.visibility = 'visible';

                }
                else
                {
                    err.innerHTML = "Invalid response: " + http.responseText;
                    err.style.visibility = 'visible';
                }
            }
            else
            {
                err.innerHTML = http.statusText + ", " + http.responseText;
                err.style.visibility = 'visible';
            }
        }
        modal.style.display = 'none';
        scanning.style.visibility="hidden";
    }
}

function scan(){
    // Hide previous error
    let error = document.getElementById("error");
    error.style.visibility = 'hidden';

    // show spinner
    let scanning = document.getElementById("scanning");
    scanning.style.visibility = "visible";

    const http = new XMLHttpRequest();
    http.open("POST", '/scan');

    http.onreadystatechange = function() {
        if (http.readyState == 4){
            if( http.status == 200)
            {
                get_wifi_json();
            }
            else
            {
                let err = document.getElementById("error");
                err.innerHTML = http.statusText + " " + http.responseText;
                err.style.visibility = 'visible';
            }
            scanning.style.visibility = "hidden";
        }
    }
    
    http.send();
}

function getNetworkInfo(){
    const http = new XMLHttpRequest();
    http.open("GET", '/connection.json');

    http.onreadystatechange = function() {
        if (http.readyState == 4){
            if( http.status == 200)
            {
                var response = JSON.parse(http.response);

                var network_div = document.getElementById("network");
                network_div.innerHTML = "SSID: " + response.ssid;

                var ip_div = document.getElementById("ip");
                ip_div.innerHTML = "IP: " + response.ip;

                document.getElementById("finishbutton").disabled = false;
            }
            else
            {
                let err = document.getElementById("error");
                err.innerHTML = http.statusText + ", " + http.responseText;
                err.style.visibility = 'visible';
            }
        }
    }

    
    http.send();
}

function showConfirmation(){
    let modal = document.getElementById('confirmation')
    modal.style.display = 'block';
}

function hideConfirmation(){
    let modal = document.getElementById('confirmation')
    modal.style.display = 'none';
}

function finishSetup(){

    const http = new XMLHttpRequest();
    http.open("POST", '/finish');
    
    http.onreadystatechange = function() {
        if (http.readyState == 4){
            if( http.status == 200)
            {
                showFinished();
            }
            else
            {
                let err = document.getElementById("error");
                err.innerHTML = http.responseText;
                err.style.visibility = 'visible';
            }
            hideConfirmation();
        }
    }

    http.send();
}

function showFinished(){
    let modal = document.getElementById('finished')
    modal.style.display = 'block';
}