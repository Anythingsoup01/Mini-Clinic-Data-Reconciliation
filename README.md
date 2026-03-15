# Mini-Clinic-Data-Reconciliation

## Platform Support

[Windows (WSL - Windows Subsystem for Linux)](#wsl2-setup-instructions)
<br>
Linux

## How to Setup

### 0. Get AI API Key
This project uses Googles Gemini AI as the backend AI. To get an API Key you must go to [AI Studio](https://aistudio.google.com/api-keys) and generate a key after logging in. This key will be used in the config setup down below.

### 1. Clone the Repository

Pull the repository and all necessary submodules using the following command:

```bash

git clone --recursive https://github.com/your-username/Mini-Clinic-Data-Reconciliation.git
cd Mini-Clinic-Data-Reconciliation

```

### 2. Initialize Configuration

Copy the template configuration file and update it with your environment-specific settings (database paths, API keys, etc.):

```bash

# Edit config.yaml with your preferred text editor
API_KEY: XXXXXXXXXXXXXXXXXXXXXXX
WEBSERVER_PORT: XXXX

```

### 3. Build with CMake

This project uses CMake for cross-platform builds. Run the following commands to compile:

```bash

mkdir build
cd build
cmake ..
cmake --build . -j N

```
### 4. Run

```bash
./build/mini-clinic
```
Go to localhost:PORT/api/home and proceed to login with admin/admin

---

## Design Decisions

### Language

This project is implemented in **C++20**. C++ was chosen for its high-performance memory management and ability to handle concurrent data processing requirements, which are critical for clinical data reconciliation.

### Libraries

* **[nlohmann/json]:** Used for robust JSON parsing, ensuring all incoming data from the web interface is validated against the schema.
* **[libcurl]** Chosen as a means of communicating with the Gemini API to send and receive data.
* **[yaml-cpp]:** Used for reading the config file and is extremely easy and versatile.

### Platform
* **[Linux/WSL]:** I chose this platform due to how easy it is to setup and run on both Windows and Linux.

### Language
* **[C++]:** I chose this language because it compiles directly into machine code, allowing for shorter wait times and predictable memory allocations.


---

# WSL2 Setup Instructions

Follow these steps to install and configure Windows Subsystem for Linux (WSL2) for development.


## 1. Enable WSL and Virtualization Features
Open **PowerShell** as an Administrator and run the following command to install the WSL kernel and the default Ubuntu distribution:

```bash
# This example is using a debian distro, such as Ubuntu
wsl --install

```

*Note: If you already have WSL 1 installed, you can ensure you are using version 2 by running:*
`wsl --set-default-version 2`

---

## 2. Restart Your Computer

Windows requires a reboot to initialize the Virtual Machine Platform and Windows Hypervisor Platform features.

---

## 3. Set Up Your Linux User

After restarting, a terminal window will open automatically to finish the installation.

1. Wait for the decompression to finish.
2. Enter a **Username** (this does not have to match your Windows username).
3. Enter a **Password**.

---

## 4. Install Build Essentials

Once inside the Linux terminal, update your package manager and install the tools needed for C++ development and CMake:

```bash
sudo apt update && sudo apt upgrade -y
sudo apt install build-essential cmake gdb git -y

```

---
