Now that we have a solid core set of functioanlity that can be rolled into the system on compilation/install and we can override or extend with cjit the next obvious step is making it somehow more meta and multi threaded.

The way we are working with this we can make a nucleus c file that is a "thread" that utilizes ak24 thread pool to issu tasks. We can make this work if we are careful about how things are used/accessed. If we make the "body" as something not ran by the main runtime but a seperate one that the module implements and has in-thread we can make a pattern where we "copy everything we need" to the thread for processing, and then "return the value" ensuring we don't have wierd threading issues. 

We could do that, OR we could update the runtime itself for sets/gets to be atomic entirely (slower af) 

we also could say "fuck it" and not concern the core with threading

# Cool

It would be cool if we could trace all modules loaded from cjit at runtime and run a special command that then run the akx --compile on the target, making an integrated binary specific for the application that is ran to ensure no cjit is used at runtime. 

Meaning debug/ dev could be done with cjit defs then we could then bundle a runtime specific to the app for "release" lmao 

