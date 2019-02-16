# Suspiria Web Framework
A simple lightweight web framework for C++ lovers like myself. I got inspiration from Django to make this.

The simple idea is that we have variety of tools but everything should be loose coupled and tightly cohesive.

Even basic things like HTTP routing can be easily ripped apart and replaced with another implementation.

1. **It's lightweight!** this ain't no Boost or Django. It's just a structure for web development.
2. **Loose coupled** components are not concrete, you can plug in and out any component. Isolation is also encouraged by the concept of the app registration.
3. **Basic encoders** are already provided using some of the libraries out things like JSON parsing. Basic template engine and all but again all of these you can throw away.
4. **Various servers** are available. You can have HTTP web server including support for web sockets or have a simple TCP server take care of your requests.

## Conclusion
I'm sure you're not going to build a business or a high scale solution using this framework.
I like C++ and I often find myself making web apis one way or another. For my own projects, I'm going to be using this.
If you're a C++ lover who wants to use C++ for their own passion only, join me, help me make this a good web framework you and I can have fun working with.
I'm designing this mostly for me and so that I can have a nice web framework to quickly make web apis.
