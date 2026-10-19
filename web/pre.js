// Prepended to the build, so the script runs on any page: it uses Module.canvas or a <canvas
// id="canvas"> when the page provides one, and otherwise adds a canvas filling the window. The
// canvas holds device pixels; CSS shows it at the window's size (see fit_canvas in main.cc).
(function () {
  var canvas = Module['canvas'] || document.getElementById('canvas');
  if (!canvas) {
    canvas    = document.createElement('canvas');
    canvas.id = 'black-hole-sim';
    // A stylesheet rule rather than inline style, which GLFW clears when it resizes the canvas.
    var style = document.createElement('style');
    style.textContent = '#black-hole-sim{position:fixed;inset:0;width:100vw;height:100vh;display:block;outline:none}';
    document.head.appendChild(style);
    (document.body || document.documentElement).appendChild(canvas);
  }
  if (!canvas.id) canvas.id = 'canvas';  // raylib addresses it by id
  canvas.tabIndex = -1;                  // focusable, for keyboard input
  canvas.addEventListener('contextmenu', function (e) { e.preventDefault(); });
  Module['canvas'] = canvas;
})();
