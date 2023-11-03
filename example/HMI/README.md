# Test WebHMI

## Install

```
npm install
```

## Run

Open Index.html with your favorite server

The live server extension for VSCode is recommended for development

## Change the connection settings

Open the file `public/src/app.js` and change the connection settings

```javascript
machine = new WEBHMI.Machine({
  port: 8000, // "Port of the server"
  ipAddress: "127.0.0.1", // "IP address of the server"
  maxReconnectCount: 5000,
});
```
