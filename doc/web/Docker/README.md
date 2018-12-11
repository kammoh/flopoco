Readme for the Flopoco dockerfile 
=================================

This Dockerfile allow the containarisation of FloPoCo.
Thanks to Luc Forget for the initial push
 
1. Install docker
On debian/ubuntu it should be ` apt install docker.io `

2. type (from the flopoco root directory):

    cd Docker
    docker build -t flopoco -f Dockerfile.4.1.2 .
    alias flopoco="docker run --rm=true -v $PWD:/flopoco_workspace flopoco"
    flopoco BuildAutocomplete



3. Remarks
 * To live on the edge with the git version, replace the corresponding line in 2 with:
    docker build -t flopoco -f Dockerfile.gitmaster .


 * On Ubuntu, the docker command below will require root access:
either replace in step 2 ` docker build ` with ` sudo docker build `
or add to step 1 ` sudo usermod -aG docker ${USER} `
