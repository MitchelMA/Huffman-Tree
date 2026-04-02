with import <nixpkgs> { };

pkgs.mkShell {
  name = "Huffman Coding";
  nativeBuildInputs = with pkgs; [
    clang
    cmake
    tmux
    gdb
    valgrind
    python3
  ];

  shellHook = ''
    export PATH=$PATH;${pkgs.clang}/bin
  '';
}
