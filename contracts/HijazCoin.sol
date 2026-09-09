// SPDX-License-Identifier: MIT
pragma solidity ^0.8.24;

import "@openzeppelin/contracts/token/ERC20/ERC20.sol";

/**
 * Hijaz Coin (HJZ)
 *
 * V1 design:
 * - ERC-20 compatible
 * - Fixed initial/max supply
 * - No interest/riba mechanism
 * - No owner mint function
 * - No arbitrary balance modification
 */
contract HijazCoin is ERC20 {

    uint256 public constant MAX_SUPPLY = 1000000000 * 10 ** 18;

    constructor(address initialHolder)
        ERC20("Hijaz Coin", "HJZ")
    {
        require(
            initialHolder != address(0),
            "Invalid initial holder"
        );

        _mint(initialHolder, MAX_SUPPLY);
    }
}
