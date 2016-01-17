for f in $(find include -name '*.?pp') $(find src -name '*.cpp'); do
    echo "Processing $f"
    gawk -i inplace '
        BEGIN {m=1}
        /BEGIN LICENSE BLOCK/ {m=0; print $0; while (getline < "LICENSE.header") print}
        /END LICENSE BLOCK/   {m=1} m {print $0}
    ' $f
done
