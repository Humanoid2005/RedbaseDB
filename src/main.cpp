#include "interpreter.h"
#include "linenoise/linenoise.h"
#include <signal.h>
#include <iostream>

static bool should_exit = false;

void sigint_handler(int signo) { should_exit = true; }

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <database>" << std::endl;
        exit(1);
    }

    signal(SIGINT, sigint_handler);

    try {
        std::cout << "\n"
                     "  ██████╗ ███████╗██████╗ ██████╗  █████╗ ███████╗███████╗\n"
                     "  ██╔══██╗██╔════╝██╔══██╗██╔══██╗██╔══██╗██╔════╝██╔════╝\n"
                     "  ██████╔╝█████╗  ██║  ██║██████╔╝███████║███████╗█████╗  \n"
                     "  ██╔══██╗██╔══╝  ██║  ██║██╔══██╗██╔══██║╚════██║██╔══╝  \n"
                     "  ██║  ██║███████╗██████╔╝██████╔╝██║  ██║███████║███████╗\n"
                     "  ╚═╝  ╚═╝╚══════╝╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝╚══════╝\n"
                     "\n"
                     "Type 'help;' for help.\n"
                     "\n";

        std::string db_name = argv[1];
        if (!SM_Manager::is_dir(db_name)) {
            SM_Manager::create_db(db_name);
        }
        SM_Manager::open_db(db_name);

        // Main input loop
        while (!should_exit) {
            char *line_read = linenoise("redbase> ");
            if (line_read == nullptr) {
                // Ctrl+D (EOF)
                break;
            }

            std::string line = line_read;
            free(line_read);

            if (should_exit) break;

            if (!line.empty()) {
                linenoiseHistoryAdd(line.c_str());

                YY_BUFFER_STATE buf = yy_scan_string(line.c_str());
                if (yyparse() == 0) {
                    if (ast::parse_tree != nullptr) {
                        try {
                            Interpreter::interpret_sql(ast::parse_tree);
                        } catch (RedBaseError &e) {
                            std::cerr << e.what() << std::endl;
                        }
                    } else {
                        should_exit = true;
                    }
                }
                yy_delete_buffer(buf);
            }
        }

        SM_Manager::close_db();
        std::cout << "Bye" << std::endl;

    } catch (RedBaseError &e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }

    return 0;
}
