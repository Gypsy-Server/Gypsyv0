

/*
 * Gypsy version 0 (basic version), open for everyone to try and give a feedback
 * 
 * 
 * 
 * 
 * 
 * 
 * Using SD card as a Fat32 File System to store content , And accessing it through our local network via ESP8266 in DHCP mode 
Router is necessary , If you don't have router then There will be a Access point mode version 
 * format sd card to FAT32 MBR before using it

It was originally a Platformio project , converted in a single file for quick upload and test
 */


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SdFat.h>

#define SD_CS_PIN 5 //IF YOU HAVE ANY OTHER SPARE GPIO USE THAT 

const char* ssid = "Enter your Network id here";
const char* password = "your network password";

ESP8266WebServer server(80);
SdFat32 SD;

volatile bool isClientStreaming = false;
volatile bool isUploading = false;
File32 upFile; 

String cleanName(String s) {
  s.replace("..", "");
  while (s.startsWith("/")) s.remove(0, 1);
  while (s.endsWith("/")) s.remove(s.length() - 1);
  const char bad[] = {'\\','/','?','%','*',':','|','"','<','>','\''};
  for (char c : bad) s.replace(String(c), "_");
  return s;
}

const char* getMimeType(const String &n) {
  String ln = n; ln.toLowerCase();
  if (ln.endsWith(".mp4")) return "video/mp4";
  if (ln.endsWith(".mkv")) return "video/x-matroska";
  if (ln.endsWith(".webm")) return "video/webm";
  if (ln.endsWith(".avi")) return "video/x-msvideo";
  if (ln.endsWith(".ogg")) return "video/ogg";
  if (ln.endsWith(".jpg") || ln.endsWith(".jpeg")) return "image/jpeg";
  if (ln.endsWith(".png")) return "image/png";
  if (ln.endsWith(".gif")) return "image/gif";
  if (ln.endsWith(".webp")) return "image/webp";
  if (ln.endsWith(".pdf")) return "application/pdf";
  if (ln.endsWith(".txt")) return "text/plain";
  return "application/octet-stream";
}
uint64_t getDirSize(const char *path) {
  uint64_t total = 0;
  File32 dir = SD.open(path);
  if (!dir) return 0;
  if (!dir.isDir()) {
    total = dir.size();
    dir.close();
    return total;
  }

  File32 entry;
  while ((entry = dir.openNextFile())) {
    char name[128];
    entry.getName(name, sizeof(name));
    if (entry.isDir()) {
      String sub = String(path);
      if (!sub.endsWith("/")) sub += "/";
      sub += String(name);
      total += getDirSize(sub.c_str());
    } else {
      total += entry.size();
    }
    entry.close();
    yield();
  }
  dir.close();
  return total;
}


void htmlHead(const String &title) {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  server.sendContent(F(
    "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<style>"
    "body{margin:0;font-family:'Lucida Console',monospace;background:#020607;color:#bdf6c6;"
    "font-size:16px;letter-spacing:0.5px;text-shadow:0 0 6px #0f4;"
    "background-image:linear-gradient(180deg,#010b0a 0%,#031511 100%);}"
    ".wrap{max-width:900px;margin:18px auto;padding:12px;text-align:center}"
    "h3{color:#6aff82;font-size:1.6em;text-shadow:0 0 10px #0f0,0 0 4px #063;"
    "letter-spacing:1px;margin-bottom:14px}"
    ".item{background:#03130c;border:2px solid #065a2a;border-radius:8px;"
    "margin:8px 0;padding:10px 12px;display:flex;justify-content:space-between;align-items:center;"
    "box-shadow:0 3px 12px rgba(0,20,0,0.6);transition:all 0.2s ease}"
    ".item:hover{background:#052512;border-color:#08b43a;box-shadow:0 4px 16px rgba(0,100,0,0.8)}"
    "form{margin:12px 0;}"
    "input[type=text],select{background:#041009;border:1px solid #0b5a2a;border-radius:6px;"
    "color:#aefcb6;padding:8px 10px;font-size:0.95em;outline:none;width:60%;"
    "box-shadow:inset 0 0 6px #053;transition:all 0.2s ease}"
    "input[type=text]:focus,select:focus{border-color:#0f5;box-shadow:0 0 8px #0f5}"
    "input[type=file]{color:#9fd9a2;background:#031006;border:1px solid #094a1c;border-radius:6px;"
    "padding:6px;font-size:0.9em;cursor:pointer;width:70%;box-shadow:inset 0 0 6px #042}"
    "input[type=file]::file-selector-button{background:#0b3418;color:#afffaf;border:none;padding:6px 10px;"
    "border-radius:6px;font-weight:bold;cursor:pointer;box-shadow:0 3px 0 #052;transition:0.2s}"
    "input[type=file]::file-selector-button:hover{background:#0f5;color:#021;box-shadow:0 0 10px #0f5}"
    "button,input[type=submit]{background:#0b3418;color:#afffaf;border:2px solid #094a1c;"
    "padding:10px 16px;border-radius:8px;font-weight:bold;font-size:1em;cursor:pointer;"
    "box-shadow:0 3px 0 #052,inset 0 0 6px #063;transition:0.2s;font-family:'Lucida Console',monospace}"
    "button:hover,input[type=submit]:hover{background:#00cc55;border-color:#0b0;color:#021;"
    "box-shadow:0 0 10px #00ff80,inset 0 0 8px #0f5;transform:translateY(-1px)}"
    ".newcat{background:#083518;border:2px solid #0a4;color:#bdf6c6;font-weight:bold;"
    "padding:10px 18px;border-radius:8px;box-shadow:0 0 10px #063,inset 0 0 5px #0a4;"
    "transition:all 0.2s ease;cursor:pointer}"
    ".drop{margin:12px 0;padding:16px;border:2px dashed #0b5a2a;border-radius:8px;"
    "background:#041008;color:#9fd9a2;transition:all 0.2s ease}"
    ".drop.dragover{background:#072a14;box-shadow:0 0 16px #0f5}"
    ".progress{width:100%;height:12px;background:#010b04;border:1px solid #083b18;border-radius:6px;margin-top:10px}"
    ".bar{height:100%;background:linear-gradient(90deg,#00ff88,#008844);width:0%;border-radius:6px;box-shadow:0 0 10px #00ff88}"
    "#sdinfo{background:#03150c;border:1px solid #0a4;border-radius:8px;padding:10px;margin:10px 0;"
    "box-shadow:0 0 10px #062;color:#bdf6c6;font-size:0.95em;text-shadow:0 0 6px #0f4;}"
    "@media(max-width:500px){body{font-size:15px}.item{flex-direction:column;align-items:flex-start}}"
    "</style><title>"
  ));
  server.sendContent(title + "</title></head><body><div class='wrap'><h3>" + title + "</h3>");
}

void htmlEnd() {
  server.sendContent(F("<p style='font-size:12px;color:#6f9f6f'>Gypsy Media Server</p></div></body></html>"));
}


void streamFile(File32 &f, const char *type) {
  WiFiClient c = server.client();
  server.setContentLength(f.size());
  server.send(200, type, "");
  uint8_t buf[1024];
  while (f.available() && c.connected()) {
    size_t n = f.read(buf, sizeof(buf));
    if (n) c.write(buf, n);
    yield();
  }
  f.close(); c.flush(); c.stop();
}

bool delRecursive(const String &p) {
  File32 d = SD.open(p.c_str());
  if (!d || !d.isDir()) { if (d) d.close(); return false; }
  File32 e; char n[64];
  while ((e = d.openNextFile())) {
    e.getName(n, sizeof(n));
    String name = p + "/" + String(n);
    if (e.isDir()) delRecursive(name);
    else SD.remove(name.c_str());
    e.close(); yield();
  }
  d.close(); SD.rmdir(p.c_str());
  return true;
}
void handleRoot() {
  File32 root = SD.open("/videos");
  if (!root || !root.isDir()) {
    server.send(200, "text/html", "<h3>/videos missing</h3>");
    return;
  }

  htmlHead("Own your DATA!");

  File32 e; char n[64];
  while ((e = root.openNextFile())) {
    if (e.isDir()) {
      e.getName(n, sizeof(n));
      server.sendContent(
        "<div class='item'><span class='fname'>" + String(n) + 
        "</span><a href='/list?cat=" + String(n) + "'><button>Open</button></a></div>");
    }
    e.close();
    yield(); 
  }

  server.sendContent(F(
    "<div class='card'><h3>Manage Categories</h3>"
    "<form id='createForm'><input name='cat' placeholder='New category name' type='text'>"
    "<input type='submit' value='Create' class='newcat'></form></div>"
    "<div class='card'><h3>Upload Files</h3>"
    "<form id='upForm' enctype='multipart/form-data'>"
    "<label>Category:</label><select name='category' id='catSel'></select><br><br>"
    "<input type='file' name='file' multiple><div id='drop' class='drop'>Drag & Drop</div>"
    "<div class='progress' id='prog'><div class='bar' id='bar'></div></div>"
    "<input type='submit' value='Upload' class='newcat'></form></div>"
    "<script>var s=document.getElementById('catSel');"
  ));

  root.rewind();
  while ((e = root.openNextFile())) {
    if (e.isDir()) {
      e.getName(n, sizeof(n));
      server.sendContent("s.innerHTML+='<option value=\"" + String(n) + "\">" + String(n) + "</option>';"); 
    }
    e.close();
    yield();
  }

  server.sendContent(F(
    "var drop=document.getElementById('drop'),bar=document.getElementById('bar'),prog=document.getElementById('prog');"
    "function send(fd,cat){var x=new XMLHttpRequest();x.open('POST','/upload?cat='+encodeURIComponent(cat));"
    "x.upload.onprogress=function(e){if(e.lengthComputable){prog.style.display='block';bar.style.width=(e.loaded/e.total*100)+'%';}};"
    "x.onload=function(){location.reload();};x.send(fd);}"
    "document.getElementById('upForm').onsubmit=function(e){e.preventDefault();var fd=new FormData(this);send(fd,this.category.value);};"
    "drop.ondragover=function(e){e.preventDefault();drop.classList.add('dragover');};"
    "drop.ondragleave=function(e){drop.classList.remove('dragover');};"
    "drop.ondrop=function(e){e.preventDefault();drop.classList.remove('dragover');var fd=new FormData();"
    "for(var f of e.dataTransfer.files)fd.append('file',f);send(fd,document.getElementById('catSel').value);};"
    "document.getElementById('createForm').onsubmit=function(e){e.preventDefault();var n=this.cat.value.trim();if(!n)return;"
    "fetch('/create?cat='+encodeURIComponent(n),{method:'POST'}).then(()=>location.reload());};"
    "</script>"
  ));
  server.sendContent(F(
    "<div id='sdinfo' class='card'>Loading SD info...</div>"
    "<script>fetch('/sdinfo').then(r=>r.text()).then(t=>{document.getElementById('sdinfo').innerHTML=t;})"
    ".catch(()=>{document.getElementById('sdinfo').innerHTML='SD info unavailable';});</script>"
  ));

  htmlEnd();
  root.close();
}

void handleCreate() {
  if (isUploading || isClientStreaming) { server.send(503, "text/plain", "Busy"); return; }
  String c = cleanName(server.arg("cat"));
  if (c.length()) {
    String p = "/videos/" + c;
    if (!SD.exists(p.c_str())) SD.mkdir(p.c_str());
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleList() {
  String cat = cleanName(server.arg("cat"));
  String path = "/videos/" + cat;
  File32 dir = SD.open(path.c_str());
  if (!dir || !dir.isDir()) { server.send(404, "text/plain", "Not found"); return; }

  htmlHead("Files in " + cat);

  File32 f; char n[64];
  while ((f = dir.openNextFile())) {
    if (!f.isDir()) {
      f.getName(n, sizeof(n));
      String nm = String(n), fp = path + "/" + nm;
      server.sendContent("<div class='item'><div class='fname'>" + nm + "</div>"
                         "<div><a href='/open?file=" + fp + "'><button>View</button></a>"
                         "<form method='POST' action='/delfile' style='display:inline'><input type='hidden' name='file' value='" + fp + "'>"
                         "<input type='submit' value='Delete'></form></div></div>");
    }
    f.close();
    yield();
  }

  server.sendContent("<form method='POST' action='/delcat'><input type='hidden' name='cat' value='" + cat + "'><input type='submit' value='Delete Category'></form>");
  htmlEnd();
  dir.close();
}

void handleOpen() {
  String f = server.arg("file");
  if (!SD.exists(f.c_str())) { server.send(404, ""); return; }
  htmlHead("Preview");
  String lf = f; lf.toLowerCase();
  if (lf.endsWith(".mp4") || lf.endsWith(".mkv") || lf.endsWith(".webm"))
    server.sendContent("<video controls autoplay><source src='/stream?file=" + f + "'></video>");
  else if (lf.endsWith(".jpg") || lf.endsWith(".png") || lf.endsWith(".gif") || lf.endsWith(".webp"))
    server.sendContent("<img src='/stream?file=" + f + "'>");
  else
    server.sendContent("<iframe src='/stream?file=" + f + "' style='height:70vh;border:none'></iframe>");
  htmlEnd();
}

void handleStream() {
  if (isUploading || isClientStreaming) { server.send(503, "text/plain", "Busy"); return; }
  String p = server.arg("file");
  if (!SD.exists(p.c_str())) { server.send(404, "Not found"); return; }

  File32 f = SD.open(p.c_str(), FILE_READ);
  const char* mime = getMimeType(p);
  server.sendHeader("Content-Disposition", "inline; filename=\"" + p.substring(p.lastIndexOf('/') + 1) + "\"");
  server.sendHeader("Cache-Control", "max-age=3600");
  isClientStreaming = true;
  streamFile(f, mime);
  isClientStreaming = false;
}

void handleDelCat() {
  if (isUploading || isClientStreaming) { server.send(503, "Busy"); return; }
  String c = cleanName(server.arg("cat"));
  delRecursive("/videos/" + c);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDelFile() {
  if (isUploading || isClientStreaming) { server.send(503, "Busy"); return; }
  String f = server.arg("file");
  SD.remove(f.c_str());
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleUploadData() {
  HTTPUpload &u = server.upload();
  if (u.status == UPLOAD_FILE_START) {
    String cat = cleanName(server.arg("cat"));
    if (!cat.length()) cat = "default";
    String folder = "/videos/" + cat;
    if (!SD.exists(folder.c_str())) SD.mkdir(folder.c_str());
    String path = folder + "/" + cleanName(u.filename);
    isUploading = true;
    upFile = SD.open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
  } else if (u.status == UPLOAD_FILE_WRITE) {
    if (upFile) upFile.write(u.buf, u.currentSize);
  } else if (u.status == UPLOAD_FILE_END || u.status == UPLOAD_FILE_ABORTED) {
    if (upFile) upFile.close();
    isUploading = false;
  }
}

void handleSdInfo() { // to get the storage information 

  uint64_t usedBytes = getDirSize("/videos");
  float usedMB = usedBytes / (1024.0 * 1024.0);

  String html;
  html.reserve(256);
  html += "<h4>SD Storage</h4>";
  html += "Used: " + String(usedMB, 1) + " MB";

  FatVolume* vol = SD.vol();
  if (vol) {
    uint32_t totalClusters = vol->clusterCount();
    if (totalClusters == 0) {
      html += "<br>Total: unknown";
      html += "<div class='progress' style='margin-top:6px;'><div class='bar' style='width:0%'></div></div>";
    } else {
      float bytesPerCluster = vol->bytesPerCluster();
      float totalMB = (totalClusters * bytesPerCluster) / (1024.0 * 1024.0);
      float pct = (totalMB > 0.0) ? ((usedMB / totalMB) * 100.0) : 0.0;
      html += " / " + String(totalMB, 1) + " MB";
      html += " (" + String(pct, 0) + "%)";
      html += "<div class='progress' style='margin-top:6px;'><div class='bar' style='width:" + String(pct) + "%'></div></div>";
    }
  } else {
    html += "<br>Total: unknown";
    html += "<div class='progress' style='margin-top:6px;'><div class='bar' style='width:0%'></div></div>";
  }

  server.send(200, "text/html", html);
}


void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("getting into the connection...");
  while (WiFi.status() != WL_CONNECTED) { delay(400); Serial.print("."); }
  Serial.println("\nIP: " + WiFi.localIP().toString());

  if (!SD.begin(SD_CS_PIN, SD_SCK_MHZ(25))) {
    Serial.println("SD is failing bro please scratch it or format it to FAT32 MBR before using it"); while (1) delay(1000);
  }
  if (!SD.exists("/videos")) SD.mkdir("/videos");

  server.on("/", HTTP_GET, handleRoot);
  server.on("/create", HTTP_POST, handleCreate);
  server.on("/list", HTTP_GET, handleList);
  server.on("/open", HTTP_GET, handleOpen);
  server.on("/stream", HTTP_GET, handleStream);
  server.on("/delcat", HTTP_POST, handleDelCat);
  server.on("/delfile", HTTP_POST, handleDelFile);
  server.on("/upload", HTTP_POST, []() { server.send(200); }, handleUploadData);
  server.on("/sdinfo", handleSdInfo);

  server.begin();
  Serial.println("HTTP server ready");
}

void loop() { server.handleClient(); yield(); }
