#include "LixieWebApi.h"

LixieWebApi::LixieWebApi() {

}







// Create an SSL certificate object from the files included above
SSLCert cert = SSLCert(
  example_crt_DER, example_crt_DER_len,
  example_key_DER, example_key_DER_len
);

// First, we create the HTTPSServer with the certificate created above
HTTPSServer secureServer = HTTPSServer(&cert);

// Additionally, we create an HTTPServer for unencrypted traffic
HTTPServer insecureServer = HTTPServer();

// Declare some handler functions for the various URLs on the server
void handleRoot(HTTPRequest * req, HTTPResponse * res);
void handle404(HTTPRequest * req, HTTPResponse * res);

void LixieWebApi::onRootRequest(HTTPRequest * req, HTTPResponse * res) {
  res->setHeader("Content-Type", "text/html");

  res->println("<!DOCTYPE html>");
  res->println("<html>");
  res->println("<head><title>Hello World!</title></head>");
  res->println("<body>");
  res->println("<h1>Hello World!</h1>");

  res->print("<p>Your server is running for ");
  res->print((int)(millis()/1000), DEC);
  res->println(" seconds.</p>");

  // You can check if you are connected over a secure connection, eg. if you
  // want to use authentication and redirect the user to a secure connection
  // for that
  if (req->isSecure()) {
    res->println("<p>You are connected via <strong>HTTPS</strong>.</p>");
  } else {
    res->println("<p>You are connected via <strong>HTTP</strong>.</p>");
  }

  res->println("</body>");
  res->println("</html>");
}

void LixieWebApi::onApiGet(HTTPRequest * req, HTTPResponse * res) {
  req->discardRequestBody();
  res->setHeader("Content-Type", "application/json");
  res->setHeader("Connection", "Keep-Alive");
  res->setHeader("Keep-Alive", "timeout=60, max=1000");
  res->println("{\"i_dont_like\": \"tacos\"}");
}

void LixieWebApi::onApiPost(HTTPRequest * req, HTTPResponse * res) {
  req->discardRequestBody();
  res->setHeader("Content-Type", "application/json");
  res->setHeader("Connection", "Keep-Alive");
  res->setHeader("Keep-Alive", "timeout=60, max=1000");
  res->println("{\"result\": \"zippity do-da\"}");
}

void LixieWebApi::onResourceNotFound(HTTPRequest * req, HTTPResponse * res) {
  req->discardRequestBody();
  res->setStatusCode(404);
  res->setStatusText("Not Found");
  res->setHeader("Content-Type", "text/html");
  res->setHeader("Connection", "Keep-Alive");
  res->setHeader("Keep-Alive", "timeout=60, max=1000");
  res->println("<!DOCTYPE html><html><head><title>Not Found</title></head><body><h1>404 Not Found</h1><p>The requested resource was not found on this server.</p></body></html>");
}

void setupWebServer() {
  // For every resource available on the server, we need to create a ResourceNode
  // The ResourceNode links URL and HTTP method to a handler function
  ResourceNode * nodeRoot = new ResourceNode("/", "GET", &this->onRootRequest);
  ResourceNode * nodeRoot = new ResourceNode("/api", "GET", &onApiGet);
  ResourceNode * nodeRoot = new ResourceNode("/api", "POST", &onApiPost);
  ResourceNode * node404  = new ResourceNode("", "GET", &handle404);

  // Add the root node to the servers. We can use the same ResourceNode on multiple
  // servers (you could also run multiple HTTPS servers)
  secureServer.registerNode(nodeRoot);
  insecureServer.registerNode(nodeRoot);

  // We do the same for the default Node
  secureServer.setDefaultNode(node404);
  insecureServer.setDefaultNode(node404);

  Serial.println("Starting HTTPS server...");
  secureServer.start();
  Serial.println("Starting HTTP server...");
  insecureServer.start();
  if (secureServer.isRunning() && insecureServer.isRunning()) {
	  Serial.println("Servers ready.");
  }
}



void pollWebServer() {
  secureServer.loop();
	insecureServer.loop();
}
