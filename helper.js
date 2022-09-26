( function (){
	var apps = document.getElementById("apps");
	if( apps )
		return;

//      fileapps = document.createElement("div");
//	fileapps.innerHTML = '\
//		<form method="POST" onsubmit="return on_submit" target="_blank" action="/upload" enctype="multipart/form-data">  \
//			<input type="file" name="file" required \>  \
//			<input type="submit" name="upload" value="upload" \>   \
//		</form> ';
	
	function upload1() {
		var f= document.getElementById("file").files[0]
		if (!f){
			window.alert("input file");
			return;
		}
		var r = new FileReader();

		r.onerror = function() {
			window.alert("read "+f.name+" err");
		}
		r.onloadend = function() {
			console.log("...........read done..");	
			//build ajax multipart header.
			var ajax = new XMLHttpRequest();
			ajax.open("POST","/upload",false);
			var boundary="---------------"+Date.now.toString(16);
			ajax.setRequestHeader("Content-type","multipart/form-data; boundary="+boundary);
			var data=new Blob(["--" + boundary + "\r\n" + "Content-Disposition: form-data; name=\"upload\"; filename=\"file.txt\"\r\n\"Content-Type: application/octet-stream\r\n\r\n",r.result,"\r\n--"+boundary+"--\r\n"],{type: 'application/octet-stream'})

			//ajax.upload.addEventListener("error",file_upload_err);
			ajax.send(data);
		}
		r.readAsDataURL(f);
		console.log(222222222);
	}
	
	function upload() {
		var f= document.getElementById("file").files[0]
		if (!f){
			window.alert("input file");
			return;
		}
		var formData = new FormData();
		formData.append(f.name,f);
			var ajax = new XMLHttpRequest();
			ajax.open("POST","/upload",false);
			ajax.setRequestHeader("Upgrade-Insecure-Requests",1);
			//ajax.setRequestHeader("Content-Type","multipart/form-data");
			ajax.setRequestHeader("Mime-Type","multipart/form-data");
			ajax.setRequestHeader("mimeType","multipart/form-data");
			var t1,count=0;
			function refresh() {
				if (count >= 10) {
					window.location = window.location;
				}
				var output =  document.getElementById("output").value;
				output += "\r\n wait to refresh " + (10-count) + " s";
				document.getElementById("output").value = output;
				count++ ; 
			}
			ajax.onreadystatechange = function() {
				if(ajax.readyState == 4 && ajax.status == 200) {
					document.getElementById("output").value = ajax.responseText;
					//set refreash..
					t1 = setInterval(refresh,1000);
				}
			} 
			ajax.send(formData);
	}
	apps = document.createElement("div");
	apps.innerHTML = `
			<tr>
			  <td>
			    <input type="file" name="file" id="file" required />  
			  </td>
			  <td>
			    <input type="submit" name="upload" value="upload" id="upload" /> 
			  </td>
			</tr>
			<hr>
			<tr>
			  <td>setup pi4 wifi mode:</td>
			  <td><input type="button" class="wifi" id="wifi_ap" value="ap" ></td>
			  <td><input type="button" class="wifi" id="wifi_sta" value="sta" > </td>
			  <td><input type="button" class="wifi" id="wifi_router" value="router"></td>
			</tr>
			<hr>
			<tr>
			  <td><textarea id="output" readonly rows="30" cols="150" > </textarea></td>
			  </br>
			  <td>shell command input:</td><td><input type="text" id="cmdinput" /></td>
		        </tr>
			<hr>`

			
			
	var body=document.getElementsByTagName("body")[0];
	//body.append(fileapps);
	body.append(apps);
	document.getElementById("upload").onclick=upload;
 

	function execute(cmd){
		var ajax = new XMLHttpRequest();
		ajax.open("POST","/execute",false);
		ajax.onreadystatechange = function() {
			if(ajax.readyState == 4 && ajax.status == 200) {
				document.getElementById("output").value = ajax.responseText;
			}
		} 
		ajax.send("cmd="+cmd);
	}

	document.getElementById("cmdinput").onkeydown=function(e){
		if(e.keyCode == 13 ){ //enter key press
			var cmdinput= document.getElementById("cmdinput");
			execute(cmdinput.value);
			cmdinput.value='';
		}
	};
	
	var wifi_buttons = document.getElementsByClassName("wifi");
	for(var i=0; i < wifi_buttons.length;i++)
	wifi_buttons[i].onclick = function(e) {
		var src = e.path[0]
		var cmd = "/workspace/pi4/pi_wifi.sh "+src.value
		execute(cmd);
	}
}) ()
