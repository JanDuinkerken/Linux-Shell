/*
Jan Duinkerken Rodríguez | Jesús Mosteiro García
jan.duinkerken@udc.es    | jesus.mosteiro@udc.es
*/

#include "shell.h"

int main(int argc, char* argv[]) {
    cmd_args args;
    args.n = argc - 1;
    args.args = &argv[1];
    c_list(args);
    return 0;
}
