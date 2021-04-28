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
    const http = new XMLHttpRequest();
    http.open("GET", '/blacklist.txt');

    http.onreadystatechange = function() {
        if (http.readyState == 4){
            if (http.status == 200){
                err.style.visibility = 'hidden'
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