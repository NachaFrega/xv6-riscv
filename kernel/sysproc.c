#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"




uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


uint64
sys_chmod(void)
{
    char path[MAXPATH];
    int mode;
    struct inode *ip;

    // Obtener argumentos
    if (argstr(0, path, MAXPATH) < 0) {
        return -1; // Error al obtener el primer argumento
    }
    if (argint(1, &mode) < 0) {
        return -1; // Error al obtener el segundo argumento
    }

    begin_op();
    if ((ip = namei(path)) == 0) {
        end_op();
        return -1; // Archivo no encontrado
    }

    ilock(ip);

    // Verificar si el archivo es inmutable
    if (ip->perm == 5) {
        iunlockput(ip);
        end_op();
        return -1; // No se pueden cambiar los permisos de un archivo inmutable
    }

    ip->perm = mode & 7; // Máscara para asegurar valores válidos (0-7)
    iupdate(ip);         // Guardar los cambios en el disco
    iunlockput(ip);
    end_op();

    return 0; // Éxito
}
