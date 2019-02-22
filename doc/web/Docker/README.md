Readme for the Flopoco dockerfile 
=================================

This Dockerfile allow the containarisation of FloPoCo.
Thanks to Luc Forget for the initial push
 
1. Install docker
On debian/ubuntu it should be ` apt install docker.io `

2a. To use the latest stable version, type (from the flopoco root directory):

    cd flopoco/doc/web/Docker
    docker build -t flopoco -f Dockerfile.4.1.2 .
    alias flopoco="docker run --rm=true -v $PWD:/flopoco_workspace flopoco"
    flopoco BuildAutocomplete

2b. To live on the edge with the git version, type (from the flopoco root directory):

    cd flopoco/doc/web/Docker
    docker build -t flopoco -f Dockerfile.gitmaster .
    alias flopoco="docker run --rm=true -v $PWD:/flopoco_workspace flopoco"
    flopoco BuildAutocomplete

3. Remarks

 * On Ubuntu, the docker command above will require root access:
either replace in step 2 ` docker build ` with ` sudo docker build `
or add to step 1 ` sudo usermod -aG docker ${USER} `
