# Nginx "Dr. Maleki" Load Balancer Module

![Nginx](https://img.shields.io/badge/Nginx-1.24.0%2B-blue?logo=nginx)

> This extension is dedicated to Dr. Maleki, the Vice Dean of Education of the Faculty of Electrical and Computer Engineering at Isfahan University of Technology. Her strategy for equalizing the capacity of course groups inspired me to develop this extension.

This is a dynamic module for Nginx that introduces a new upstream load balancing algorithm named `dr_maleki`.

This algorithm is designed for scenarios where you want to sequentially fill up servers to a certain capacity before moving to the next, and then dynamically raise that capacity for all servers once they are full.

## The "Dr. Maleki" Algorithm

The logic of the `dr_maleki` algorithm is as follows:

1.  An initial request `limit` is set for all upstream servers (e.g., 100 active connections).
2.  All new requests are sent to the **first** server in the upstream list until its connection count reaches the `limit`.
3.  Nginx then moves to the **second** server and sends all new requests there until it also reaches the `limit`.
4.  This process continues sequentially for all servers in the list.
5.  Once **all** servers have reached the current `limit`, the `limit` for all servers is increased by 10%.
6.  The entire process repeats, starting again from the first server with the new, higher limit.

## Prerequisites

Before compiling, you need to have the following installed on your system:

* The **Nginx source code**. The version of the source code **must** match the version of Nginx you have installed.
* A C compiler and build tools (e.g., `gcc`, `make`).
* The development libraries required to build Nginx from source (PCRE, OpenSSL, zlib, etc.).

You can install the basic build tools with:

```bash
# For Debian / Ubuntu
sudo apt-get install -y build-essential

# For CentOS / RHEL / Fedora
sudo yum groupinstall -y "Development Tools"
```

## Build and Installation Instructions

Follow these steps carefully to compile and install the module.

### Step 1: Clone the Repository

Clone this repository to your machine.

```bash
git clone [https://github.com/your-username/dr_maleki_balancer.git](https://github.com/your-username/dr_maleki_balancer.git)
cd dr_maleki_balancer
```

### Step 2: Get the Nginx Source Code

Download the Nginx source code that **exactly matches your installed Nginx version**.

First, check your current Nginx version:
```bash
nginx -v
```

Then, download the matching source. For example, if you have version `1.24.0`:
```bash
cd /tmp
wget [https://nginx.org/download/nginx-1.24.0.tar.gz](https://nginx.org/download/nginx-1.24.0.tar.gz)
tar -zxvf nginx-1.24.0.tar.gz
```

### Step 3: Find Nginx's Original Configure Arguments

For binary compatibility, a dynamic module must be compiled with the same flags as the Nginx binary it will be loaded into. Get your current Nginx configuration flags by running:

```bash
nginx -V
```
Copy the entire output line that begins with `configure arguments:`.

### Step 4: Configure and Build the Module

Navigate to the Nginx source directory and run the `./configure` script with the arguments from the previous step, adding this module at the end.

```bash
cd /tmp/nginx-1.24.0/

# PASTE your configure arguments and add the --add-dynamic-module flag
./configure [PASTE_ARGUMENTS_HERE] --add-dynamic-module=/path/to/dr_maleki_balancer

# Compile the modules
make modules
```
**Example:**
```bash
./configure --prefix=/etc/nginx --with-http_ssl_module --with-compat --add-dynamic-module=/home/user/dr_maleki_balancer
```


### Step 5: Copy the Module File

Copy the compiled module (`.so` file) to your Nginx modules directory. The location may vary, but it's often `/usr/lib/nginx/modules/`.

```bash
sudo cp objs/ngx_http_upstream_dr_maleki_module.so /usr/lib/nginx/modules/
```

## Configuration

Finally, enable the module in your `nginx.conf` file.

### Step 1: Load the Module

Add the following line at the top of your `/etc/nginx/nginx.conf` file.

```nginx
load_module modules/ngx_http_upstream_dr_maleki_module.so;
```

### Step 2: Use the Algorithm in an Upstream Block

In the `http` section of your configuration, define an `upstream` block and activate the algorithm by adding the `dr_maleki;` directive.

```nginx
http {
    upstream my_backend {
        # Activate the Dr. Maleki load balancing algorithm
        dr_maleki;

        server backend1.example.com;
        server backend2.example.com;
        server backend3.example.com;
    }

    server {
        listen 80;

        location / {
            proxy_pass http://my_backend;
        }
    }
}
```

### Step 3: Test and Restart Nginx

Test your configuration and restart the Nginx service.

```bash
sudo nginx -t
sudo systemctl restart nginx
```

Your custom load balancer is now active!

## License

This project is licensed under the MIT License.