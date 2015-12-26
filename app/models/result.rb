class Result < ApplicationRecord
  belongs_to :snippet
  belongs_to :compiler
end
