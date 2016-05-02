# poe: online [ruby] environment

sandbox (C):

* seccomp: to restrict syscallls
* cgroup (memory, pids): to limit resources
* overlayfs

backend (Rust):

* Iron: webapp framework

frontend (TypeScript):

* Angular 2

## Installation

**仮想マシン上で実行することを強くお勧めします**

~~~sh
% git clone -b ⚙ https://github.com/rhenium/poe.git && cd poe

% cd sandbox
% vi config.h config_seccomp.h
% make && sudo make install  # => sandbox/runner
% cd ..

% cd backend
% cargo build --release # => target/release/poe
% cd ..

% cd frontend
% npm i
% rake deploy # => frontend/target/
% cd ..

% nvim config.json
% mkdir -p /path/to/datadir/{env/base,snippets}

% sudo pacstrap -cd /path/to/datadir/env/base filesystem bash openssl coreutils shadow libxml2
% sudo arch-chroot /path/to/datadir/env/base useradd -m unyapoe -u 27627 # home directory is needed

% rake 'ruby[2.3.0]' # will add to config.json
% rake 'php[7.0.0]'

% ./backend/target/release/poe config.json
% # setup reverse proxy: /api/ => backend, * => frontend
~~~

## License
MIT License
