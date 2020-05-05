var status_nozzle = document.querySelector(".status>[type=nozzle]");
var status_bed = document.querySelector(".status>[type=bed]");
var printingSpeed = document.querySelector(".status>[type=printingSpeed]");
var printingFlow = document.querySelector(".status>[type=printingFlow]");
var filamentMaterial = document.querySelector(".status>[type=filamentMaterial]");
var filenames = document.querySelectorAll(".filename");
var progress = document.querySelectorAll(".progress>div");
var percents = document.querySelectorAll(".percents");


function nav_click(event){
    document.querySelector(".content[active]").removeAttribute("active");
    document.querySelector("nav>span[active]").removeAttribute("active");

    let name = this.getAttribute("target");
    document.querySelector(".content[name="+name+"]")
    .setAttribute("active", "active");
    this.setAttribute("active", "active")
}

function on_update_printer(xhr){
    if (xhr.status != 200) {
        return;
    }

    let data = JSON.parse(xhr.responseText);
    let nozzle = data["temperature"]["tool0"];
    status_nozzle.querySelector("span").innerText =
    Math.round(nozzle["actual"]) + "/" + Math.round(nozzle["target"]) + "°C";
    let offset = nozzle["target"] - nozzle["actual"];
    if (offset > 2){
        status_nozzle.querySelector("div>[warming]").style.visibility = "visible";
    } else {
        status_nozzle.querySelector("div>[warming]").style.visibility = "hidden";
    }
    if (offset < -2 && nozzle["actual"] > 49){
        status_nozzle.querySelector("div>[cooling]").style.visibility = "visible";
    } else {
        status_nozzle.querySelector("div>[cooling]").style.visibility = "hidden";
    }

    let bed = data["temperature"]["bed"];
    status_bed.querySelector("span").innerText =
    Math.round(bed["actual"]) + "/" + Math.round(bed["target"]) + "°C";
    offset = bed["target"] - bed["actual"];
    if (offset > 2){
        status_bed.querySelector("div>[warming]").style.visibility = "visible";
    } else {
        status_bed.querySelector("div>[warming]").style.visibility = "hidden";
    }

    if ((offset < -2) && (bed["actual"] > 49)){
        status_bed.querySelector("div>[cooling]").style.visibility = "visible";
    } else {
        status_bed.querySelector("div>[cooling]").style.visibility = "hidden";
    }

    let printing_speed = data["print_settings"]["printing_speed"];
    printingSpeed.querySelector("span").innerText =
    printing_speed + "%";

    let flow_factor = data["print_settings"]["flow_factor"];
    printingFlow.querySelector("span").innerText = 	flow_factor + "%";

    let filament_Material = data["print_settings"]["filament_material"];
    filamentMaterial.querySelector("span").innerText = filament_Material;
};

function on_update_job(xhr){
    if (xhr.status != 200) {
        return;
    }

    let data = JSON.parse(xhr.responseText);
    let completion = data["progress"]["precent_done"] || 0;
    progress.forEach(function(elm){
        elm.style.width = completion+"%";
    });
    percents.forEach(function(elm){
        elm.innerText = completion+"%";
    });

    let filename= data["file"];
    if (filenames[0].innerText != filename){
        filenames.forEach(function(elm){
            elm.innerText = filename;
        });
    }
}

function ajax_get(url, callback){
    let xhr = new XMLHttpRequest();
    xhr.open("GET", url, true);
    xhr.setRequestHeader("Content-Type", "application/json");
    xhr.setRequestHeader("X-Api-Key", "2790249435FB4C838E08DDE663402E7F");
    xhr.onreadystatechange = function(){
        if (xhr.readyState != 4){
            return;
        }
        callback(xhr);
    };
    xhr.send();
}

function update_status(){
    ajax_get("/api/printer", on_update_printer)
    ajax_get("/api/job", on_update_job);
}

setInterval(update_status, 100);

document.querySelectorAll("nav>span:not([disabled])").forEach(function(elm){
    elm.addEventListener("click", nav_click);
});
