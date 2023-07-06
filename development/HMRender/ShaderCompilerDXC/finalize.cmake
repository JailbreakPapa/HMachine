# Make sure this project is built when the Editor is built
wd_add_as_runtime_dependency(ShaderCompilerDXC)

wd_add_dependency("ShaderCompiler" "ShaderCompilerDXC")