function loadDoc() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
       var obj = JSON.parse(this.responseText)
       console.log(obj)
       document.getElementById("result").innerHTML = obj.id;
      }
    };
    xhttp.open("GET", "/hello", true);
    xhttp.send();
}