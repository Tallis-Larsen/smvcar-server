# SMVCar-Server

### Setup

This server software has been designed to be compatible with Digital Ocean's [App Platform](https://www.digitalocean.com/products/app-platform). This makes it much easier to deploy an instance of the server without any physical hardware, port forwarding, or server knowledge. Clicking the button below will send you to Digital Ocean's instance creation page with some of the parameters already filled in:

[![Deploy to Digital Ocean](https://www.deploytodo.com/do-btn-blue.svg)](https://cloud.digitalocean.com/apps/new?repo=https://github.com/Tallis-Larsen/smvcar-server/tree/main)

After you're redirected, it's pretty much as easy as logging in, clicking `Create app`, and adding a billing method.

### Operation

The server should build and deploy automatically. It would be a good idea to go to the app dashboard and restart the server before a race, though.

The app dashboard also provides the link to the HTML client interface. This is the same link you will use on the Raspberry Pi client to connect to the server, except with `wss://` instead of `https://`. Example:

`https://smvcar-server-iesb4.ondigitalocean.app/`
becomes:
`wss://smvcar-server-iesb4.ondigitalocean.app/`

Don't get confused: the HTML client (the one the "pit crew" uses) will still be accessed through the normal https link.

### Modification

The server (like any other open-source project) can be modified if more functionality is needed later. To do this, you will need to [fork](https://github.com/Tallis-Larsen/smvcar-server/fork) this GitHub repository and edit [`.do/deploy.template.yaml`](https://github.com/Tallis-Larsen/smvcar-server/blob/main/.do/deploy.template.yaml) to use the new repository name. You will also need to modify your Digital Ocean App Platform deployment to use this new repository as well. App Platform will automatically build and deploy any changes you push to `main` by default. I am not going to explain how git works in this short readme, because if you don't know, you shouldn't be making changes.


