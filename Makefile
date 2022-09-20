# ONNX_MLIR_EXE = path to onnx-mlir
# ONNX_MLIR_INCLUDE = path that contains OnnxMlirRuntime.h
# ONNX_MLIR_LIB = path that contains libcruntime.a

all : libclip_visual.so libclip_textual.so

visual.onnx :
	wget https://clip-as-service.s3.us-east-2.amazonaws.com/models/onnx/ViT-B-32/visual.onnx

textual.onnx :
	wget https://clip-as-service.s3.us-east-2.amazonaws.com/models/onnx/ViT-B-32/textual.onnx

%.o : %.onnx
	$(ONNX_MLIR_EXE) --EmitObj -O3 --parallel $^

libclip_visual.so : libclip_visual.c libclip_visual.h libclip_visual.version visual.o
	gcc -fPIC -shared -o $@ libclip_visual.c visual.o -I $(ONNX_MLIR_INCLUDE) -lm -L $(ONNX_MLIR_LIB) -l:libcruntime.a -Wl,--version-script=libclip_visual.version

libclip_textual.so : libclip_textual.c libclip_textual.h trie.c trie.h libclip_textual.version textual.o
	gcc -fPIC -shared -o $@ libclip_textual.c trie.c textual.o -I $(ONNX_MLIR_INCLUDE) -lm -L $(ONNX_MLIR_LIB) -l:libcruntime.a -l:libpcre2-8.a -Wl,--version-script=libclip_textual.version