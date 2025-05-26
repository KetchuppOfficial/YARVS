{
    description = "YARVS: RV64I functional simulator";

    inputs = {
        nixpkgs.url = "github:nixos/nixpkgs/nixos-24.11";
    };

    outputs = {
        nixpkgs,
        ...
    }: let
        system = "x86_64-linux";
    in {
        devShells."${system}".default = let
            pkgs = import nixpkgs {
                inherit system;
            };
        in pkgs.mkShell {
            buildInputs = with pkgs; [
                fmt
                elfio
                cli11
                gtest
            ];
            nativeBuildInputs = with pkgs; [
                cmake
                python3
            ];
        };
    };
}
