[![DOI](https://zenodo.org/badge/1239250468.svg)](https://doi.org/10.5281/zenodo.20191444)
# Raijin Coupler

Raijin Coupler is a C++ implementation of the JcupLT coupling library.

## Repository layout

- src: source code
- docs/manual: LaTeX source of the manual
- docs/manual/figs: figures used by the manual
- docs/api: Doxygen configuration for API documentation
- test: test programs and execution directories

## Build the manual

Run:

    cd docs/manual
    make

This generates docs/manual/Raijin.pdf.

The generated PDF is not tracked by git.

## Build the API documentation

Run:

    cd docs/api
    make

This generates docs/api/html/index.html.

Generated HTML files are not tracked by git.

## Dependencies

For the manual:

- TeX Live
- latexmk
- dvipdfmx
- BibTeX

For API documentation:

- Doxygen
- Graphviz

On Fedora:

    sudo dnf install -y texlive-scheme-full latexmk doxygen graphviz

## License

This software is distributed under the BSD 3-Clause License.

Raijin Coupler is derived from the Jcup/JcupLT coupling library.
The original Jcup copyright notice is retained in the LICENSE file.

## Versioning

Raijin Coupler follows the Jcup/JcupLT-style version numbering rule.

The version format is:

    M.IIBBFF

where:

- M is the major version.
- II is incremented when the public interface changes.
- BB is incremented when the internal behavior changes.
- FF is incremented for bug fixes.

When an upper-level version number is incremented, all lower-level version
numbers are reset to zero.

Examples:

- 0.010000: initial interface release
- 0.020000: interface change
- 0.020100: internal behavior change
- 0.020101: bug fix
- 1.000000: major version update

The GitHub/Zenodo release v0.5 was created before this formal versioning rule
was introduced. Formal Jcup-style versioning starts from Version 0.010000.

## Citation

Please cite the Zenodo archive when using Raijin Coupler.

DOI: 10.5281/zenodo.20191445
