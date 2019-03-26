# My Shell!
Something fun to hack away with, and a good excuse to use C!

## TODO 
- [x] Piping outputs! -> https://www.gnu.org/software/libc/manual/html_node/Creating-a-Pipe.html#Creating-a-Pipe
- [x] Pipe an arbitray number of outputs together
- [ ] Set up some kind of streaming? It looks like commands that have logn
  outputs are not getting sent to the next process...?
- [ ] Implement shell built ins such as `cd`
- [ ] Implement up arrow scrolling back to old commands (requires refactor of
  parsing stdin in the loop)
- [ ] Set up a path
- [x] Run everything blocking unless you give it some reason not to
