{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  name = "SheduleOneCalculator";
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
