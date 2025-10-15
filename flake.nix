{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/b841d90d33396d34143e79ac31393da1df6527fe";
  };

  outputs = { self, nixpkgs }:
  let
    system = "x86_64-linux";

    pkgs = import nixpkgs {
      inherit system;
    };
  in {
    devShells.${system}.default = pkgs.mkShell {
      nativeBuildInputs = [
        pkgs.tinycc
        pkgs.gdb
      ];

      buildInputs = [ ];
    };
  };
}
