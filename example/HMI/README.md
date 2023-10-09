To start a new hmi using the widget system

```
lpm init
lpm install widget-template
```

If you want to install the local gizmo by file add the directory:
```
"@loupeteam/widgets-[mygizmo]": "file:../../src/hmi"
```
Run npm install

```
//Example local file module depencency
{
  "name": "hmi",
  "version": "1.0.0",
   ...
  "dependencies": {
    "@loupeteam/widgets-mygizmo": "file:../../src/hmi"
  }
}
```

If you want to install widgets from a package run lpm install
```
lpm install widgets-[mygizmo]
```
