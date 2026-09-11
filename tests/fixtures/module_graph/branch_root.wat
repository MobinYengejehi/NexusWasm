(module
    (import
        "left"
        "value"
        (func $left
            (result i32)
        )
    )

    (import
        "right"
        "value"
        (func $right
            (result i32)
        )
    )

    (func
        (export "run")
        (result i32)

        call $left
        call $right
        i32.add
    )
)
