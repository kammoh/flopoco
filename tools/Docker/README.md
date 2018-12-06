Readme for the Flopoco dockerfile - Thanks to Luc Forget for the initial push

This Dockerfile allow the containarisation of FloPoCo.

1/ Install docker

2/ type (from the flopoco root directory):

cd Docker
docker build -t flopoco -f Dockerfile.4.1.2 .
alias flopoco="docker run --rm=true -v $PWD:/flopoco_workspace flopoco"
flopoco BuildAutocomplete



3/ Remarks
On Ubuntu, the docker command below will require root access:
either replace in step 2/ "docker build" with "sudo docker build"
or add to step 1/ "sudo usermod -aG docker ${USER}"
