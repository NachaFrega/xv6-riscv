
# Informe Tarea 0 Ignacia Frega

Inicialmente, intenté instalar xv6 en WSL, una característica de Windows que permite ejecutar un entorno Linux dentro del sistema operativo sin necesidad de una máquina virtual completa. El proceso avanzó sin problemas hasta la instalación del toolchain, donde encontré varias dificultades. A pesar de seguir las instrucciones de instalación, surgieron inconvenientes, especialmente en la etapa de ejecución de make qemu. En este punto, decidí probar con VirtualBox utilizando Ubuntu para ver si obtenía mejores resultados, y finalmente logré completar la instalación con éxito.

#### Primer comando utilizado:
```
sudo apt-get update
```
#### Segundo comando:
```
sudo apt-get upgrade
```
#### Tercer comando:
```
sudo apt install git build-essential qemu-system
```
#### Cuarto comando:
```
sudo apt-get install build-essential gdb-multiarch
```
Después empecé con toolchain
```
git clone https://github.com/riscv/riscv-gnu-toolchain
cd riscv-gnu-toolchain
```
```
./configure --prefix=/opt/riscv --with-arch=rv64gc --with-abi=lp64d
sudo make
```
Agregar el toolchain al PATH:
```
export PATH=/opt/riscv/bin:$PATH
```
#### Finalmente volver a xv6-riscv y ejecutar:
```
make qemu
```
#### Adjunto capturas de pantalla del xv6 funcionando:



