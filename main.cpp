#include <stdio.h>

#include "common.h"
#include "llama.h"

int main(int argc, const char* argv[]) {
    fprintf(stderr, "hello, llama.cpp!\n");
    if (argc < 2) return 1;

    const char* model_path = argv[1];
    const char* prompt = argv[2];
    fprintf(stderr, "model_path: %s\n", model_path);
    fprintf(stderr, "prompt: %s\n", prompt);

    // init backend
    gpt_params params;
    params.n_ctx = 2048;
    params.n_gpu_layers = 99;
    llama_backend_init();
    llama_numa_init(params.numa);

    // init model
    llama_model_params model_params = llama_model_params_from_gpt_params(params);
    llama_model* model = llama_load_model_from_file(model_path, model_params);

    if (model == NULL) {
        fprintf(stderr, "load model failed\n");
        return 1;
    }

    // init context
    llama_context_params ctx_params = llama_context_params_from_gpt_params(params);
    llama_context* ctx = llama_new_context_with_model(model, ctx_params);

    if (ctx == NULL) {
        fprintf(stderr, "create context failed\n");
        return 1;
    }

    // tokenize prompt
    std::vector<llama_token> tokens_list;
    tokens_list = ::llama_tokenize(ctx, prompt, true);
    fprintf(stderr, "prompt tokens: ");
    for (auto token : tokens_list) fprintf(stderr, "%d ", token);
    fprintf(stderr, "\n");

    // calculate context size
    const int n_predict = 128;
    const int n_ctx = llama_n_ctx(ctx);
    const int n_kv_req = tokens_list.size() + (n_predict - tokens_list.size());

    if (n_kv_req > n_ctx) {
        fprintf(stderr, "sequence length too long\n");
        return 1;
    }

    // evaluate the initial prompt
    llama_batch batch = llama_batch_init(512, 0, 1);
    for (size_t i = 0; i < tokens_list.size(); i++)
        llama_batch_add(batch, tokens_list[i], i, {0}, false);

    // inference prefilling
    batch.logits[batch.n_tokens - 1] = true;

    if (llama_decode(ctx, batch) != 0) {
        fprintf(stderr, "decode failed\n");
        return 1;
    }

    int n_decode = 0;
    int n_cur = batch.n_tokens;
    std::vector<llama_token> decode_tokens;

    // autoregressive decoding
    while (n_cur <= n_predict) {
        auto n_vocab = llama_n_vocab(model);
        auto* logits = llama_get_logits_ith(ctx, batch.n_tokens - 1);

        std::vector<llama_token_data> candidates;
        candidates.reserve(n_vocab);
        for (llama_token token_id = 0; token_id < n_vocab; token_id++)
            candidates.emplace_back(llama_token_data{token_id, logits[token_id], 0.0f});

        // perform greedy decoding
        auto c_data = candidates.data();
        auto c_size = candidates.size();
        llama_token_data_array candidates_p = {c_data, c_size, false};
        const llama_token new_token_id = llama_sample_token_greedy(ctx, &candidates_p);
        decode_tokens.push_back(new_token_id);

        // if end of generation
        if (llama_token_is_eog(model, new_token_id) || n_cur == n_predict) break;

        // push new token
        llama_batch_clear(batch);
        llama_batch_add(batch, new_token_id, n_cur, {0}, true);

        n_decode++;
        n_cur++;

        // inference decoding
        if (llama_decode(ctx, batch)) {
            fprintf(stderr, "decode failed\n");
            return 1;
        }
    }

    fprintf(stderr, "decode tokens: ");
    for (auto token : decode_tokens) fprintf(stderr, "%d ", token);
    fprintf(stderr, "\n");

    auto result = llama_detokenize(ctx, decode_tokens).c_str();
    fprintf(stderr, "result: %s\n", result);

    // free memory
    llama_batch_free(batch);
    llama_free(ctx);
    llama_free_model(model);
    llama_backend_free();

    return 0;
}
