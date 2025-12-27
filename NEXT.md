First functions to seed the runtime with:

Right now eval earched and walks builtins, but it doesnt have a symbol map or ay way to define symbols.

This means we will define, in a jit target, the actual core eval/ runtime logic of the process. 


    
```
(cjit-load-builtin ctx      :root "bootstrap/ctx.c")
    ; the context is a scoping mechanism in ~/.ak24/context.h

(cjit-load-builtin fn       :root "bootstrap/fn.c")
    ; could use ~/.ak24/lambda.h and a static fn

(cjit-load-builtin compile  :root "bootstrap/compile.c") 
    ; could run lists and compile to a dir

(cjit-load-builtin load_store :root "boostrap/load_store.c")
    ; uses `@` for singular LOAD _AND_ STORE


(@ x 3)     ; set x 3           - return cell as if it was a get (pointer). copies "3" cell TO x (deep copy)
(@ x)       ; get x             - returns pointer to cell
(@ x 3 5)   ; compare-and-swap  - compare x with 3 and set to 5 if 3 is val, returns get on target as the other two do


```