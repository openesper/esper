function loadQueries(){
    let http = new XMLHttpRequest();
    http.open("GET", "/querylog.csv");

    http.onreadystatechange = function() {
        if (http.readyState == 4){
            if (http.status == 200)
            {
                console.log(http.response)
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
    http.open("GET", 'blacklist.txt');

    http.onreadystatechange = function() {
        if (http.readyState == 4){
            if (http.status == 200){
                err.style.visibility = 'hidden'
                list = http.response.split('\n');
                console.log(list);
                
                // Remove old list
                let tb = document.getElementsByTagName('tbody')[0];
                while(tb.firstChild)
                {
                    tb.removeChild(tb.firstChild);
                }

                // Add new list
                for(let i = 0; i < list.length; i++){
                    let row = tb.insertRow();
                    let domain = row.insertCell();
                    let deleteButton = row.insertCell();

                    domain.innerHTML = list[i];

                    let button = document.createElement('button');
                    button.innerHTML = '&times;';
                    button.onclick = function(){updateBlacklist("delete", list[i])};
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
    updateBlacklist("add", urlinput.value)
}

function updateBlacklist(action, hostname){
    let err = document.getElementById('error');
    let http = new XMLHttpRequest();
    http.open('POST', `/blacklist?action=${action}`);

    http.onreadystatechange = function(){
        if (http.readyState == 4){
            console.log(http.status)
            if (http.status == 200){
                err.style.visibility = 'hidden';
                loadBlacklist();
            }
            else{
                
                err.innerHTML = http.responseText;
                err.style.visibility = 'visible';
            }
        }
    };
    http.send(hostname);
}