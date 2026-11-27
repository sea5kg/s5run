// NoSQL database provider example.
//
// Usage: ./go-nosql-provider <host:port>
//
// A simple in-memory document store served as an s5run provider:
// insert (JSON documents), find (exact match on key fields),
// update and delete by id.

package main

import (
	"fmt"
	"os"
	"strconv"
	"strings"

	"s5runprovider/s5runprovider"
)

// ---- document store ----

type record struct {
	id  string
	doc s5runprovider.Obj
}

type MyDB struct {
	s5runprovider.DbBase
	collections map[string][]record
	nextID      int
}

func newMyDB() *MyDB {
	return &MyDB{collections: map[string][]record{}}
}

// Insert adds a JSON document to the collection, returns the assigned id.
func (db *MyDB) Insert(collection string, doc s5runprovider.S5RunVariable) string {
	obj, ok := doc.(s5runprovider.Obj)
	if !ok {
		panic("db.insert: expected an object document")
	}
	db.nextID++
	id := fmt.Sprintf("doc-%d", db.nextID)
	db.collections[collection] = append(db.collections[collection], record{id: id, doc: obj})
	return id
}

// Find returns documents where every filter field matches exactly.
// An empty filter matches all documents in the collection.
func (db *MyDB) Find(collection string, filter s5runprovider.S5RunVariable) []s5runprovider.S5RunVariable {
	f, ok := filter.(s5runprovider.Obj)
	if !ok {
		panic("db.find: expected an object filter")
	}
	out := []s5runprovider.S5RunVariable{}
	for _, rec := range db.collections[collection] {
		if matchFilter(rec.doc, f) {
			// copy with the id attached
			cp := s5runprovider.Obj{}
			for k, v := range rec.doc {
				cp[k] = v
			}
			cp["_id"] = s5runprovider.Str(rec.id)
			out = append(out, cp)
		}
	}
	return out
}

// Update replaces the document with the given id.
func (db *MyDB) Update(collection string, id string, doc s5runprovider.S5RunVariable) bool {
	obj, ok := doc.(s5runprovider.Obj)
	if !ok {
		panic("db.update: expected an object document")
	}
	for i, rec := range db.collections[collection] {
		if rec.id == id {
			db.collections[collection][i].doc = obj
			return true
		}
	}
	return false
}

// Delete removes the document with the given id.
func (db *MyDB) Delete(collection string, id string) bool {
	recs := db.collections[collection]
	for i, rec := range recs {
		if rec.id == id {
			db.collections[collection] = append(recs[:i], recs[i+1:]...)
			return true
		}
	}
	return false
}

// matchFilter: every filter key must exist in doc with an equal value.
func matchFilter(doc s5runprovider.Obj, filter s5runprovider.Obj) bool {
	for k, fv := range filter {
		dv, ok := doc[k]
		if !ok || !valueEqual(dv, fv) {
			return false
		}
	}
	return true
}

func valueEqual(a, b s5runprovider.S5RunVariable) bool {
	switch at := a.(type) {
	case s5runprovider.Int:
		bt, ok := b.(s5runprovider.Int)
		return ok && at == bt
	case s5runprovider.Str:
		bt, ok := b.(s5runprovider.Str)
		return ok && at == bt
	case s5runprovider.Bool:
		bt, ok := b.(s5runprovider.Bool)
		return ok && at == bt
	case s5runprovider.Float:
		bt, ok := b.(s5runprovider.Float)
		return ok && at == bt
	}
	return false
}

// ---- main ----

func main() {
	if len(os.Args) < 2 {
		fmt.Fprintf(os.Stderr, "Usage: %s <host:port>\n", os.Args[0])
		os.Exit(1)
	}
	host := "127.0.0.1"
	port := 7777
	{
		hostPort := os.Args[1]
		colon := strings.LastIndex(hostPort, ":")
		if colon < 0 {
			fmt.Fprintf(os.Stderr, "Usage: %s <host:port>\n", os.Args[0])
			os.Exit(1)
		}
		host = hostPort[:colon]
		port, _ = strconv.Atoi(hostPort[colon+1:])
	}

	fmt.Printf("== go-provider: NoSQL database on %s:%d\n", host, port)

	ctx := s5runprovider.NewContext()
	ctx.AddObject(newMyDB())

	if err := s5runprovider.Serve(ctx, host, port); err != nil {
		fmt.Printf("!!! FAIL: %v\n", err)
		os.Exit(1)
	}
	fmt.Println("go-provider: done")
}
