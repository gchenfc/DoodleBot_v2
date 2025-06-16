#pragma once

#include "gcode_player.h"

void handleFileUpload(AsyncWebServerRequest* request, String filename,
                      size_t index, uint8_t* data, size_t len, bool final);
void handlePrintGcode(AsyncWebServerRequest* request);

static const auto kUploadPage PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>DoodleBot Gcode Upload</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; }
    input[type="file"] { margin: 20px; }
    input[type="submit"] { padding: 10px 20px; }
  </style>
</head>
<body>
  <h1>DoodleBot Gcode Upload</h1>
  <form action="/upload" method="post" enctype="multipart/form-data">
    <input type="file" name="file" accept=".gcode,.gz" required>
    <input type="submit" value="Upload">
  </form>
  <p>Upload a gcode file to the DoodleBot!  Accepted file types: .gcode, .gz</p>
</body>
</html>
)rawliteral";
static const auto kSuccessPage PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Upload Successful</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; }
    h1 { color: green; }
  </style>
</head>
<body>
  <h1>Upload Successful!</h1>
  <p>Your file has been uploaded successfully.</p>
  <a href="/upload">Upload another file</a>
</body>
</html>
)rawliteral";

class LineByLineParser {
 public:
  LineByLineParser(const std::function<void(std::string_view)> line_handler,
                   char delimiter = '\n')
      : line_handler_(line_handler), delimiter_(delimiter) {}

  bool parseContent(std::string_view input) {
    // First manually handle the leftovers
    if (!leftovers_.empty()) {
      size_t pos = input.find(delimiter_);
      if (pos == std::string_view::npos) {
        leftovers_ += input;  // No delimiter found, append to leftovers
        return false;         // Indicate that parsing is incomplete
      }
      leftovers_ += input.substr(0, pos);
      input.remove_prefix(pos + 1);
      line_handler_(leftovers_);  // Handle the leftovers
      leftovers_.clear();         // Clear leftovers after handling
    }
    // Continue parsing the rest
    {
      size_t end_pos = input.rfind(delimiter_);
      if (end_pos != std::string_view::npos) {
        // If the last part ends with a delimiter, we can process it
        leftovers_ = input.substr(end_pos + 1);
        input.remove_suffix(input.size() - end_pos);
      } else {
        leftovers_ = input;  // Save the entire input as leftovers
        input.remove_suffix(input.size());  // Clear input
      }
      line_handler_(input);
    }
    return leftovers_.empty();
  }
  bool parseRemaining() {
    if (leftovers_.empty()) {
      return true;  // Nothing to parse
    }
    line_handler_(leftovers_);
    leftovers_.clear();
    return true;
  }

 private:
  std::function<void(std::string_view)>
      line_handler_;            // Callback for line handling
  char delimiter_;              // Default delimiter for line parsing
  std::string leftovers_ = "";  // Leftover after each chunk, before newline
};

LineByLineParser gcode_line_parser(
    [](std::string_view line) { gcode_player.loadLine(line); }, '\n');

void setupUi() {
  server.on("/upload", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", kUploadPage);
  });
  // post to the upload page
  server.on(
      "/upload", HTTP_POST, [](AsyncWebServerRequest*) {}, handleFileUpload);
  server.on("/testing", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->redirect("/success.html");
    WebSerial.println("Testing endpoint hit, redirecting to success page.");
  });
  server.on("/success.html", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", kSuccessPage);
  });
  server.on("/print", HTTP_GET, handlePrintGcode);
}

void updateUi() {}

void handleFileUpload(AsyncWebServerRequest* request, String filename,
                      size_t index, uint8_t* data, size_t len, bool final) {
  WebSerial.printf("Received chunk %zu of %zu bytes.  Final? %d\n", index, len,
                   final);

  // Unused filename
  (void)filename;

  // Initialize on first chunk
  if (!index) {
    gcode_player.startUpload();
  }

  // Handle reading
  if (gcode_player.isUploading()) {
    std::string_view input(reinterpret_cast<const char*>(data), len);
    gcode_line_parser.parseContent(input);
  } else {
    request->send(500, "text/plain",
                  "500: Upload not started, cannot write data");
    return;
  }

  // This was the final chunk
  if (final) {
    if (gcode_player.isUploading()) {
      gcode_player.endUpload();
      request->redirect("/success.html");
    } else {
      request->send(500, "text/plain",
                    "500: something went wrong with the upload");
    }
  }

  WebSerial.printf("Finished Received chunk %zu of %zu bytes.  Final? %d\n",
                   index, len, final);
}

void handlePrintGcode(AsyncWebServerRequest* request) {
  auto* response = request->beginChunkedResponse(
      "text/plain",
      [&](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
        return gcode_player.printProgram(reinterpret_cast<char*>(buffer),
                                         maxLen, index);
      });
  // Don't download as a file.  Instead, display in browser:
  response->addHeader("X-Content-Type-Options", "nosniff");
  request->send(response);
}
