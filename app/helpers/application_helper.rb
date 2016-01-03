module ApplicationHelper
  def application_name
    Rails.application.class.to_s.split("::").first
  end
end
