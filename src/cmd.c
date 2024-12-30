// SPDX-License-Identifier: BSD-3-Clause

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>

#include "cmd.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define READ 0
#define WRITE 1

void file_descriptor(char *src, int dst, int flags) {
  if (src == NULL)
    return;

  int src_fd = open(src, flags, 0644);

  if (src_fd < 0)
    return;

  dup2(src_fd, dst);
  close(src_fd);
}

void both_redirections(word_t *out_err) {
  if (out_err == NULL)
	 return;
  char *file = get_word(out_err);
  int flags = O_WRONLY | O_CREAT | O_TRUNC;

  file_descriptor(file, STDERR_FILENO, flags);
  flags |= O_APPEND;
  file_descriptor(file, STDOUT_FILENO, flags);
  free(file);
}

void input_redirection(word_t *in) {
  if (in == NULL)
    return;
  char *file = get_word(in);
  int flags = O_RDONLY | O_CREAT;
  file_descriptor(file, STDIN_FILENO, flags);
  free(file);
}

void output_redirection(word_t *out, int io_flags) {
  if (out == NULL)
    return;
  char *file = get_word(out);
  int flags = O_WRONLY | O_CREAT;

  if (io_flags & IO_OUT_APPEND)
    flags |= O_APPEND;
  else
    flags |= O_TRUNC;

  file_descriptor(file, STDOUT_FILENO, flags);
  free(file);
}

void error_redirection(word_t *err, int io_flags) {
  if (err == NULL)
    return;
  char *file = get_word(err);
  int flags = O_WRONLY | O_CREAT;

  if (io_flags & IO_ERR_APPEND)
    flags |= O_APPEND;
  else
    flags |= O_TRUNC;

  file_descriptor(file, STDERR_FILENO, flags);
  free(file);
}

void command_redirections(simple_command_t *s) {
  if (s->err && s->out && s->err == s->out) {
    both_redirections(s->out);
  } else {
    input_redirection(s->in);
    output_redirection(s->out, s->io_flags);
    error_redirection(s->err, s->io_flags);
  }
}

/**
 * Internal change-directory command.
 */
static bool shell_cd(word_t *dir) {
   char *path = get_word(dir);
   return chdir(path);
}

/**
 * Internal exit/quit command.
 */
static int shell_exit(void) {
  /* TODO: Execute exit/quit. */
  return SHELL_EXIT;
}

/**
 * Parse a simple command (internal, environment variable assignment,
 * external command).
 */
static int parse_simple(simple_command_t *s, int level, command_t *father) {
  /* TODO: Sanity checks. */

  if (s == NULL)
    return -1;
  if (s->verb == NULL)
    return -1;

  /* TODO: If builtin command, execute the command. */

  char *cmd_name = get_word(s->verb);
  if (cmd_name == NULL)
    return -1;

  if (strcmp(cmd_name, "cd") == 0) {
    int saved_stdout = dup(STDOUT_FILENO);
    int saved_stderr = dup(STDERR_FILENO);
        
    command_redirections(s);
    int ret = shell_cd(s->params);
        
    dup2(saved_stdout, STDOUT_FILENO);
    dup2(saved_stderr, STDERR_FILENO);
    close(saved_stdout);
    close(saved_stderr);
        
    free(cmd_name);
    return ret;
  }
  if (strcmp(cmd_name, "exit") == 0) {
    free(cmd_name);
    return shell_exit();
  }
  

  /* TODO: If variable assignment, execute the assignment and return
   * the exit status.
   */

  /* TODO: If external command:
   *   1. Fork new process
   *     2c. Perform redirections in child
   *     3c. Load executable in child
   *   2. Wait for child
   *   3. Return exit status
   */

  int size;
  char **argv = get_argv(s, &size);
  if (argv == NULL) {
    free(cmd_name);
    return -1;
  }

  pid_t pid = fork();

  if (pid < 0) {
    free(argv);
    return -1;
  }

  if (pid == 0) {
    command_redirections(s);
    execvp(argv[0], argv);
    exit(1);
  }

  int status;
  waitpid(pid, &status, 0);

  free(argv);
  free(cmd_name);
  return WEXITSTATUS(status);
}

/**
 * Process two commands in parallel, by creating two children.
 */
static bool run_in_parallel(command_t *cmd1, command_t *cmd2, int level,
                            command_t *father) {
  /* TODO: Execute cmd1 and cmd2 simultaneously. */

  return true; /* TODO: Replace with actual exit status. */
}

/**
 * Run commands by creating an anonymous pipe (cmd1 | cmd2).
 */
static bool run_on_pipe(command_t *cmd1, command_t *cmd2, int level,
                        command_t *father) {
  /* TODO: Redirect the output of cmd1 to the input of cmd2. */

  return true; /* TODO: Replace with actual exit status. */
}

/**
 * Parse and execute a command.
 */
int parse_command(command_t *c, int level, command_t *father) {
  /* TODO: sanity checks */
  int status;
  if (c == NULL)
    return -1;

  if (c->op == OP_NONE) {
    /* TODO: Execute a simple command. */
    if (c->scmd == NULL)
      return -1;
    return parse_simple(c->scmd, level, c);
  }

  switch (c->op) {
  case OP_SEQUENTIAL:
    /* TODO: Execute the commands one after the other. */
	 status = parse_command(c->cmd1, level + 1, c);
	 return parse_command(c->cmd2, level + 1, c);	
    break;

  case OP_PARALLEL:
    /* TODO: Execute the commands simultaneously. */
    break;

  case OP_CONDITIONAL_NZERO:
    /* TODO: Execute the second command only if the first one
     * returns non zero.
     */
	 status = parse_command(c->cmd1, level + 1, c);
    if (status != 0)
      return parse_command(c->cmd2, level + 1, c);
    return status;
    break;

  case OP_CONDITIONAL_ZERO:
    /* TODO: Execute the second command only if the first one
     * returns zero.
     */
	 status = parse_command(c->cmd1, level + 1, c);
    if (status == 0)
      return parse_command(c->cmd2, level + 1, c);
    return status;
    break;

  case OP_PIPE:
    /* TODO: Redirect the output of the first command to the
     * input of the second.
     */
    break;

  default:
    return SHELL_EXIT;
  }

  return 0; /* TODO: Replace with actual exit code of command. */
}
