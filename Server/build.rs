use protobuf_codegen::Codegen;

fn main() {
    Codegen::new()
        .pure()
        .cargo_out_dir("message")
        .input("../Protobufs/message.proto")
        .include("../Protobufs")
        .run_from_script();
}
