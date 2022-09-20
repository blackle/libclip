# ONNX_MLIR_EXE = path to onnx-mlir
# ONNX_MLIR_INCLUDE = path that contains OnnxMlirRuntime.h
# ONNX_MLIR_LIB = path that contains libcruntime.a

all : libclip_visual.so libclip_textual.so clip_test

release : libclip.zip

libclip.zip : libclip_visual.so libclip_textual.so include/libclip_visual.h include/libclip_textual.h Readme.md
	zip $@ $^

src/visual.onnx :
	wget -P src https://clip-as-service.s3.us-east-2.amazonaws.com/models/onnx/ViT-B-32/visual.onnx

src/textual.onnx :
	wget -P src https://clip-as-service.s3.us-east-2.amazonaws.com/models/onnx/ViT-B-32/textual.onnx

src/%.o : src/%.onnx
	$(ONNX_MLIR_EXE) --EmitObj -O3 --parallel $^

src/trie_data.inc : src/trie_gen.py
	src/trie_gen.py > src/trie_data.inc

libclip_visual.so : src/libclip_visual.c include/libclip_visual.h src/libclip_visual.version src/visual.o
	gcc -fPIC -shared -o $@ src/libclip_visual.c src/visual.o -I include -I src -I $(ONNX_MLIR_INCLUDE) -lm -L $(ONNX_MLIR_LIB) -l:libcruntime.a -Wl,--version-script=src/libclip_visual.version -g

libclip_textual.so : src/libclip_textual.c include/libclip_textual.h src/trie.c src/trie.h src/tokenizer.c src/tokenizer.h src/trie_data.inc src/libclip_textual.version src/textual.o
	gcc -fPIC -shared -o $@ src/libclip_textual.c src/trie.c src/tokenizer.c src/textual.o -I include -I src -I $(ONNX_MLIR_INCLUDE) -lm -L $(ONNX_MLIR_LIB) -l:libcruntime.a -l:libpcre2-8.a -Wl,--version-script=src/libclip_textual.version -g

clip_test : test/clip_test.c libclip_visual.so libclip_textual.so
	gcc -o clip_test test/clip_test.c -lclip_textual -lclip_visual -L . -Wl,-rpath -Wl,. -I include -lm -pthread