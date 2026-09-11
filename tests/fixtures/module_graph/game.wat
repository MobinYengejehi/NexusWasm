(module
    (import
        "math"
        "add"
        (func $add
            (param i32 i32)
            (result i32)
        )
    )

    (func
        (export "run")
        (result i32)

        i32.const 20
        i32.const 22
        call $add
    )
)
