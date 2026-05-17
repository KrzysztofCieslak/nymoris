#ifndef LLM_H
#define LLM_H

int llm_generate(const char *model_path, const char *prompt, char *output, int max_output_len, int max_tokens);

#endif
