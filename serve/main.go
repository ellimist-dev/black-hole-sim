// Serves the web build for local testing: static files, never cached, with each request logged.
package main

import (
	"flag"
	"log"
	"net/http"
	"time"
)

func main() {
	dir := flag.String("dir", "build-web", "directory to serve")
	addr := flag.String("addr", "localhost:8080", "address to listen on")
	flag.Parse()

	files := http.FileServer(http.Dir(*dir))
	handler := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		start := time.Now()
		if r.URL.Path == "/" {
			r.URL.Path = "/black-hole-sim.html"
		}
		w.Header().Set("Cache-Control", "no-store")
		files.ServeHTTP(w, r)
		log.Printf("%s %s %s", r.Method, r.URL.Path, time.Since(start).Round(time.Microsecond))
	})

	log.Printf("serving %s at http://%s", *dir, *addr)
	log.Fatal(http.ListenAndServe(*addr, handler))
}
