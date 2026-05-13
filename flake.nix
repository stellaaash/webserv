{
  description = "A Nix-flake-based C++ development environment";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-25.11";
  };

  outputs = { self , nixpkgs ,... }: let
    # system should match the system you are running on
    system = "x86_64-linux";
  in {
    devShells."${system}".default = let
      pkgs = import nixpkgs { inherit system; };
    in pkgs.mkShell {
      packages = with pkgs; [
        gnumake
        # clangd must come from a wrapped clang-tools, with libcxx disabled
        (lib.hiPrio clang-tools)
        # The actual clang for compilation, wrapped against libstdc++
        llvmPackages_21.libstdcxxClang
        valgrind
        python3
        siege
        massif-visualizer
        lldb
      ];

      shellHook = ''
        echo "Time to web and serv."
      '';
    };
  };
}
