## **World's First Blockchain and the Boost Engine for Regulation and Security.**

![Build Status](https://codebuild.us-east-2.amazonaws.com/badges?uuid=eyJlbmNyeXB0ZWREYXRhIjoiTFFYVEF1UDVXaVZrWGNUOVlKSnphcElOMFBzZUFjZ0QwZHpoNCtseVdFdTVoa3hHeWpOQ1ZzWk51bUVHTXlIRjk4Z1d4UFJrUmVyQ2xVaWhHSkxabURJPSIsIml2UGFyYW1ldGVyU3BlYyI6IkFIWFJNOHZsVjZGOThuVzQiLCJtYXRlcmlhbFNldFNlcmlhbCI6MX0%3D&branch=master)

Welcome to the jmzkChain source code repository!

jmzkChain is developed and maintained by Hangzhou Yuliankeji Team.

This code is under rapid development. If you have any questions or advices, feel free to open an issue.

## SDKs

If your need is to develop applications based on jmzkChain, `JavaScript SDK` is provided and maintained officially. It's suitable for the usage on web, backend and mobile platforms like Android and iOS.

- [JaveScript SDK](https://github.com/jmzkChain/jmzkjs)

## Resources

1. [jmzkChain Website](https://www.vastchain.cn/)

## Supported Operating Systems

jmzkChain currently supports the following operating systems:

1. Amazon 2017.09 and higher
2. Centos 7
3. Fedora 25 and higher (Fedora 27 recommended)
4. Mint 18
5. Ubuntu 16.04 and higher (Ubuntu 18.04 recommended)
6. MacOS Darwin 10.12 and higher (MacOS 10.13.x recommended)

## For Production

The blockchain RPC interface is not designed for the Internet but for local network. And since RPC interface don't provide features like rate limitation's, security checks and so on. It highly suggests anyone who wants to run a node to use a reverse proxy server like nginx to serve all the requests.
